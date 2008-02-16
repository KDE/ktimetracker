#!/bin/sh
# This is a test script for ktimetracker. To make sure ktimetracker run correctly, run this script. Most probably, you will 
# only run this script if you modified the source code of ktimetracker.
# This test script tests time booking

testfile="/tmp/testktimetracker1.ics"
killall ktimetracker
rm $testfile

# start ktimetracker and make sure its dbus interface is ready
ktimetracker $testfile & while ! qdbus org.kde.ktimetracker /KTimeTracker version; do i=5; done

qdbus org.kde.ktimetracker /KTimeTracker org.kde.ktimetracker.ktimetracker.addTask Task1
TASKID=`qdbus org.kde.ktimetracker /KTimeTracker org.kde.ktimetracker.ktimetracker.taskIdsFromName Task1|grep -m 1 ".*"`
RVAL=`qdbus org.kde.ktimetracker /KTimeTracker org.kde.ktimetracker.ktimetracker.bookTime $TASKID 20050619Tabc 360 2>/dev/null`

EXPECTED=5
if [ "$RVAL" == "$EXPECTED" ]; then 
  echo "PASS $0"
  exit 0;
else 
  echo "FAIL $0: got /$RVAL/, expected /$EXPECTED/"
  exit 1;
fi
