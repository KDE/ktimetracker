#!/bin/sh

# This is a test script for ktimetracker. To make sure ktimetracker runs correctly, run this script. Most probably, you will 
# only run this script if you modified the source code of ktimetracker.
# check if deleting a task works
# you still need to set Settings->Configure KTimeTracker->Prompt before deleting tasks to false, but this should be fixed soon.

testfile="/tmp/testktimetracker1.ics"
killall ktimetracker
rm $testfile

# start ktimetracker and make sure its dbus interface is ready
ktimetracker $testfile & while ! qdbus org.kde.ktimetracker /KTimeTracker version; do i=5; done

qdbus org.kde.ktimetracker /KTimeTracker org.kde.ktimetracker.ktimetracker.addTask Task1
TASKID=`qdbus org.kde.ktimetracker /KTimeTracker org.kde.ktimetracker.ktimetracker.taskIdsFromName Task1|grep -m 1 ".*"`

# add a subtask blah
qdbus org.kde.ktimetracker /KTimeTracker org.kde.ktimetracker.ktimetracker.addSubTask blah $TASKID

qdbus org.kde.ktimetracker /KTimeTracker org.kde.ktimetracker.ktimetracker.deleteTask $TASKID

RVAL=`qdbus org.kde.ktimetracker /KTimeTracker version`

if [ "$RVAL" == "" ]; then 
  echo "FAIL $0: got no version."
  exit 1;
else 
  echo "PASS $0"
  exit 0;
fi
