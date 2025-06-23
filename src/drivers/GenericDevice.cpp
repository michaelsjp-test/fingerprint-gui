/*
 * SPDX-FileCopyrightText: © 2008-2016 Wolfgang Ullrich <w.ullrich@n-view.net>
 * SPDX-FileCopyrightText: 🄯 2021 Peter J. Mello <admin@petermello.net.>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR MPL-2.0
 *
 * Project "Fingerprint GUI": Services for fingerprint authentication on Linux
 * Module: GenericDevice.cpp, GenericDevice.h
 * Purpose: A device driver wrapper for generic fingerprint devices handled by
 *          libfprint
 *
 * @author Wolfgang Ullrich
 */

#include "GenericDevice.h"
#include "DeviceHandler.h"
#include "FingerprintData.h"
#include "UsbDevice.h"
#include <iostream>
#include <libusb-1.0/libusb.h>
#include <pthread.h>
#include <string>
#include <glib.h>
#include <glib-object.h>

using namespace std;

GenericDevice::GenericDevice(FpDevice *fp, USBDevice *knownUSBDevices)
    : FingerprintDevice() {
  fpDevice = fp;
  fpData = nullptr;
  identifyData = nullptr;
  dev = nullptr;
  vendorId = 0;
  productId = 0;

  driverName = string(fp_device_get_driver(fpDevice));
  displayName = string(fp_device_get_name(fpDevice));
  syslog(LOG_INFO, "initializing libfprint device driver: %s", driverName.data());
}

// Public getters and setters
string *GenericDevice::getDisplayName(int mode) {
  if (mode == DISPLAY_DRIVER_NAME)
    return &driverName;
  return &displayName;
}

void GenericDevice::setMode(int m) { mode = m; }

void GenericDevice::setIdentifyData(FingerprintData *iData) {
  int i, numPrints;
  FingerprintData *d;
  for (d = iData, numPrints = 0; d != nullptr; numPrints++) {
    d = d->next;
  }
  if (numPrints == 0) {
    syslog(LOG_ERR, "FIXME: numPrints=0.");
    return;
  }
  identifyData = g_ptr_array_new_with_free_func((GDestroyNotify)g_object_unref);
  for (d = iData; d != nullptr; d = d->next) {
    GError *error = nullptr;
    FpPrint *p = fp_print_deserialize((const guchar *)d->getData(), d->getSize(),
                                      &error);
    if (error) {
      syslog(LOG_ERR, "Failed to deserialize print: %s", error->message);
      g_error_free(error);
      continue;
    }
    g_ptr_array_add(identifyData, p);
  }
}

bool GenericDevice::canIdentify() {
  bool rc = false;
  if (fpDevice == nullptr) { // Oops!
    syslog(LOG_ERR, "FIXME: fpDevice nullptr.");
    return rc;
  }
  if (!fpDevOpen(fpDevice)) {
    return rc;
  }
  if (fp_device_has_feature(dev, FP_DEVICE_FEATURE_IDENTIFY))
    rc = true;
  fpDevClose();
  return rc;
}

int GenericDevice::getData(void **d, struct fp_pic_data **pic) {
  gsize size = 0;
  if (nullptr != pic)
    *pic = &fpPic;
  guchar *data = nullptr;
  GError *error = nullptr;
  if (!fp_print_serialize(fpData, &data, &size, &error)) {
    syslog(LOG_ERR, "Could not serialize fpData: %s", error ? error->message : "unknown");
    if (error)
      g_error_free(error);
    return 0;
  }
  *d = data;
  g_object_unref(fpData);
  fpData = nullptr;
  return (int)size;
}

void GenericDevice::setData(void *d, int size) {
  if (fpData != nullptr) {
    g_object_unref(fpData);
    fpData = nullptr;
  }
  GError *error = nullptr;
  fpData = fp_print_deserialize((const guchar *)d, size, &error);
  if (error) {
    syslog(LOG_ERR, "Failed to deserialize print data: %s", error->message);
    g_error_free(error);
    fpData = nullptr;
  }
}

// Here we go...
void GenericDevice::run() {
  syslog(LOG_DEBUG, "Starting %s.",
         (mode == MODE_VERIFY    ? "verify"
          : mode == MODE_ACQUIRE ? "acquire"
                                 : "identify"));
  switch (mode) {
  case MODE_VERIFY:
    verify();
    break;
  case MODE_ACQUIRE:
    acquire();
    break;
  case MODE_IDENTIFY:
    identify();
    break;
  }
  syslog(LOG_DEBUG, "Thread ended normally.");
}

void GenericDevice::stop() {
  if (dev == nullptr)
    return;
  fpDevClose();
}

// Open the fingerprint_device
bool GenericDevice::fpDevOpen(FpDevice *fpDevice) {
  if (fpDevice == nullptr) {
    syslog(LOG_ERR, "FIXME: fpDevice nullptr.");
    return false;
  }
  GError *error = nullptr;
  if (!fp_device_open_sync(fpDevice, nullptr, &error)) {
    syslog(LOG_ERR, "Could not open fpDevice: %s", error ? error->message : "unknown");
    if (error)
      g_error_free(error);
    return false;
  }
  dev = fpDevice;
  return true;
}

bool GenericDevice::fpDevClose() {
  if (!dev) {
    syslog(LOG_ERR, "FIXME: dev nullptr (fpDevClose).");
    return false;
  }
  syslog(LOG_DEBUG, "FP_DEV_CLOSE.");
  GError *error = nullptr;
  if (!fp_device_close_sync(dev, nullptr, &error)) {
    syslog(LOG_ERR, "Failed to close device: %s", error ? error->message : "unknown");
    if (error)
      g_error_free(error);
  }
  dev = nullptr;
  return true;
}

void GenericDevice::setTimeout(bool) {
  // Nothing to do here
}

// Run a verification task
bool GenericDevice::verify() {
  gboolean match = FALSE;
  FpPrint *scan = nullptr;

  if (fpDevice != nullptr) {
    if (!fpDevOpen(fpDevice)) {
      syslog(LOG_ERR, "Could not open generic device (verify).");
      emit noDeviceOpen();
      return false;
    }
  }
  emit neededStages(1);
  emit verifyResult(RESULT_SWIPE, &fpPic);
  GError *error = nullptr;
  if (!fp_device_verify_sync(dev, fpData, nullptr, nullptr, nullptr, &match,
                             &scan, &error)) {
    syslog(LOG_ERR, "Verify failed: %s", error ? error->message : "unknown");
    if (error)
      g_error_free(error);
    emit verifyResult(RESULT_VERIFY_RETRY, &fpPic);
    fpDevClose();
    emit matchResult(-1, &fpPic);
    return false;
  }
  if (scan) {
    FpImage *img = fp_print_get_image(scan);
    if (img)
      img_to_pixmap(img);
    g_object_unref(scan);
  } else {
    img_to_pixmap(nullptr);
  }
  emit verifyResult(match ? RESULT_VERIFY_MATCH : RESULT_VERIFY_NO_MATCH, &fpPic);
  fpDevClose();
  emit matchResult(match ? 0 : -1, &fpPic);
  return true;
}

// Run an enrollment task
bool GenericDevice::acquire() {
  mode = MODE_ACQUIRE;
  if (fpDevice != nullptr) {
    if (!fpDevOpen(fpDevice)) {
      syslog(LOG_ERR, "Could not open generic device (acquire).");
      emit noDeviceOpen();
      return false;
    }
  }

  emit neededStages(1);
  emit acquireResult(RESULT_SWIPE);
  GError *error = nullptr;
  fpData = fp_device_enroll_sync(dev, nullptr, nullptr, nullptr, nullptr, &error);
  if (error) {
    syslog(LOG_ERR, "Acquire failed: %s", error->message);
    g_error_free(error);
    fpDevClose();
    emit acquireResult(RESULT_ENROLL_FAIL);
    return false;
  }
  if (fpData) {
    FpImage *img = fp_print_get_image(fpData);
    if (img)
      img_to_pixmap(img);
  } else {
    img_to_pixmap(nullptr);
  }
  emit acquireResult(RESULT_ENROLL_COMPLETE);
  fpDevClose();
  return true;
}

bool GenericDevice::identify() {
  long match = -1L;

  if (identifyData == nullptr) {
    syslog(LOG_ERR, "No data to identify.");
    return false;
  }
  if (fpDevice != nullptr) {
    if (!fpDevOpen(fpDevice)) {
      syslog(LOG_ERR, "Could not open generic device (identify).");
      if (fpData != nullptr) {
        g_object_unref(fpData);
        fpData = nullptr;
      }
      emit noDeviceOpen();
      return false;
    }
  }

  FpPrint *match_print = nullptr;
  FpPrint *scan = nullptr;
  GError *error = nullptr;
  if (!fp_device_identify_sync(dev, identifyData, nullptr, nullptr, nullptr,
                               &match_print, &scan, &error)) {
    syslog(LOG_ERR, "Identify failed: %s", error ? error->message : "unknown");
    if (error)
      g_error_free(error);
    fpDevClose();
    emit matchResult(-1, &fpPic);
    return false;
  }
  if (scan) {
    FpImage *img = fp_print_get_image(scan);
    if (img)
      img_to_pixmap(img);
    g_object_unref(scan);
  } else {
    img_to_pixmap(nullptr);
  }
  if (match_print) {
    for (guint i = 0; i < identifyData->len; i++) {
      FpPrint *p = (FpPrint *)g_ptr_array_index(identifyData, i);
      if (fp_print_equal(match_print, p)) {
        match = i;
        break;
      }
    }
    g_object_unref(match_print);
  }
  fpDevClose();
  if (fpData != nullptr) {
    g_object_unref(fpData);
    fpData = nullptr;
  }
  syslog(LOG_DEBUG, "Match result %ld.", match);
  emit matchResult(match, &fpPic);
  return true;
}
