#! /bin/sh
$EXTRACTRC `find . -name \*.ui` >>  rc.cpp
$EXTRACTRC `find . -name \*.rc` >> rc.cpp
$EXTRACTRC `find . -name \*.kcfg` >> rc.cpp
$XGETTEXT *.h *.cpp -o $podir/ktimetracker.pot
