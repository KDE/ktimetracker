#!/bin/sh

rm -f rc.cpp
$EXTRACTRC `find . -name \*.ui` >> rc.cpp
$EXTRACTRC `find . -name \*.rc` >> rc.cpp
$EXTRACTRC `find . -name \*.kcfg` >> rc.cpp
$XGETTEXT `find . -name \*.h -o -name \*.cpp` -o $podir/ktimetracker.pot
