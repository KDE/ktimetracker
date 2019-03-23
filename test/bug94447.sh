#!/bin/sh
# This is a test script for ktimetracker. To make sure ktimetracker run correctly, run this script. Most probably, you will 
# only run this script if you modified the source code of ktimetracker.
# Create files relative to current directory if no "/" prefix
# in file name given on command line

testfile="testktimetracker1.ics"
testtodo="testtodo"
killall ktimetracker

# start ktimetracker and make sure its dbus interface is ready
cd /tmp
rm $testfile
ktimetracker $testfile & while ! qdbus org.kde.ktimetracker /KTimeTracker version; do i=5; done

qdbus org.kde.ktimetracker /KTimeTracker org.kde.ktimetracker.ktimetracker.addTask $testtodo

RVAL=1
if [ -e $testfile ]; then RVAL=0; fi

if [ $RVAL -eq 0 ]
then 
  echo "PASS $0"
  exit 0
else 
  echo "FAIL $0"
  exit 1
fi
