#!/bin/sh

# This is a test script for ktimetracker. To make sure ktimetracker run correctly, run this script. Most probably, you will 
# only run this script if you modified the source code of ktimetracker.
# First test, just check version.

testfile="/tmp/testktimetracker1.ics"
killall ktimetracker
rm $testfile

# start ktimetracker and make sure its dbus interface is ready
ktimetracker $testfile & while ! qdbus org.kde.ktimetracker /KTimeTracker version; do i=5; done

version=$(qdbus org.kde.ktimetracker /KTimeTracker version)
if [ "x" != "x$version" ]; then 
  echo "PASS $0"
  exit 0;
else 
  echo "FAIL $0: got //, expected version"
  exit 1;
fi
