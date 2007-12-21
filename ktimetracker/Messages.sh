#! /bin/sh
$EXTRACTRC `find . -name \*.ui` >>  rc.cpp
$EXTRACTRC `find . -name \*.rc` >> rc.cpp
$EXTRACTRC `find . -name \*.kcfg` >> rc.cpp
$XGETTEXT *.cpp -o $podir/ktimetracker.pot
