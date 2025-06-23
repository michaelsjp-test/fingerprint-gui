/*
 * SPDX-FileCopyrightText: © 2008-2016 Wolfgang Ullrich <w.ullrich@n-view.net>
 * SPDX-FileCopyrightText: 🄯 2021 Peter J. Mello <admin@petermello.net.>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR MPL-2.0
 *
 * Project "Fingerprint GUI": Services for fingerprint authentication on Linux
 * Module: FingerprintGUI.cpp
 * Purpose: Main for fingerprint-gui
 *
 * @author Wolfgang Ullrich
 */

#include <QtGui>
#include <QDir>
#include <QFile>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <string>

#include "DeviceHandler.h"
#include "MainWindowImpl.h"
#include "i18nHelper.h"
#include "ui_FingerprintGUI.h"

using namespace std;

bool debugTest = false;
string syslogIdent = string(GUI_NAME);

int main(int argc, char *argv[]) {
  if (!qEnvironmentVariableIsSet("XDG_RUNTIME_DIR")) {
    QString runtimeDir = QDir::temp().filePath(QString("runtime-%1").arg(getuid()));
    QDir().mkpath(runtimeDir);
    QFile::setPermissions(runtimeDir, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
    qputenv("XDG_RUNTIME_DIR", runtimeDir.toUtf8());
  }
  QApplication app(argc, argv);

  openlog(syslogIdent.data(), LOG_NDELAY | LOG_PID, LOG_AUTH);
  setlogmask(LOG_UPTO(LOG_ERR));
  for (int i = 0; i < argc; i++) {
    if ((strcmp(argv[i], ARG_DEBUG1) == 0) |
        (strcmp(argv[i], ARG_DEBUG2) == 0) |
        (strcmp(argv[i], ARG_DEBUG3) == 0)) {
      setlogmask(-1);
      debugTest = true;
    }
  }
  syslog(LOG_INFO, "Started.");
  loadTranslations(app);
  MainWindowImpl mainWindow;
  mainWindow.deviceHandler->rescan();

  mainWindow.show();
  int rc = app.exec();

  mainWindow.deviceHandler->release();
  return rc;
}
