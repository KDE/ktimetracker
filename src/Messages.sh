#!/bin/sh

# SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>
# SPDX-FileCopyrightText: 2019 Pino Toscano <pino@kde.org>
# SPDX-License-Identifier: CC0-1.0

$EXTRACTRC `find . -name \*.ui` >> rc.cpp
$EXTRACTRC `find . -name \*.rc` >> rc.cpp
$EXTRACTRC `find . -name \*.kcfg` >> rc.cpp
$XGETTEXT `find . -name \*.h -o -name \*.cpp` -o $podir/ktimetracker.pot
rm -f rc.cpp
