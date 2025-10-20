/*
 * SPDX-FileCopyrightText: © 2008-2016 Wolfgang Ullrich <w.ullrich@n-view.net>
 * SPDX-FileCopyrightText: 🄯 2021 Peter J. Mello <admin@petermello.net.>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR MPL-2.0
 *
 * Project "Fingerprint GUI": Services for fingerprint authentication on Linux
 * Module: Fingercodes.h
 * Purpose: Defines numbering of fingers
 *
 * @author Wolfgang Ullrich
 */

#ifndef _FINGERCODES_H
#define _FINGERCODES_H

#include "Globals.h"

// This translates my own finger codes like represented in "fingers[]" to the
// enum FpFinger
static const FpFinger fingerCode[10] = {
    FP_FINGER_LEFT_LITTLE,
    FP_FINGER_LEFT_RING,
    FP_FINGER_LEFT_MIDDLE,
    FP_FINGER_LEFT_INDEX,
    FP_FINGER_LEFT_THUMB,
    FP_FINGER_RIGHT_THUMB,
    FP_FINGER_RIGHT_INDEX,
    FP_FINGER_RIGHT_MIDDLE,
    FP_FINGER_RIGHT_RING,
    FP_FINGER_RIGHT_LITTLE};

#endif /* _FINGERCODES_H */
