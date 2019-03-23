#!/bin/sh

# This is a test script for ktimetracker. To make sure ktimetracker run correctly, run this script. Most probably, you will 
# only run this script if you modified the source code of ktimetracker.
# I cannot get this test to work reliably!
# I suspect the culprit is FAM.
#   -- Mark

testfile="/tmp/testktimetracker1.ics"
killall ktimetracker
rm $testfile

# start ktimetracker and make sure its dbus interface is ready
ktimetracker $testfile & while ! qdbus org.kde.ktimetracker /KTimeTracker version; do i=5; done

TODO_NAME=$0
TODO_UID=abc-123
TODO_TIME=`date +%Y%m%dT%H%M%SZ`

sleep 1 # make sure kdirstat can recognize the change in last change date

cat >> $testfile << endl
BEGIN:VCALENDAR
PRODID:-//K Desktop Environment//NONSGML KArm Test Scripts//EN
VERSION:2.0

BEGIN:VTODO
DTSTAMP:$TODO_TIME
ORGANIZER;CN=Anonymous:MAILTO:nobody@nowhere
CREATED:$TODO_TIME
UID:$TODO_UID
SEQUENCE:0
LAST-MODIFIED:$TODO_TIME
SUMMARY:$TODO_NAME
CLASS:PUBLIC
PRIORITY:5
PERCENT-COMPLETE:0
END:VTODO

END:VCALENDAR
endl

# wait so FAM and KDirWatcher tell karm and karm refreshes view
sleep 2

RVAL=`qdbus org.kde.ktimetracker /KTimeTracker org.kde.ktimetracker.ktimetracker.taskIdsFromName $TODO_NAME |grep -m 1 ".*"`
echo "RVAL = $RVAL"


# check that todo was found
if [ "$RVAL" == "$TODO_UID" ]; then 
  echo "PASS $0"
  exit 0;
else 
  echo "FAIL $0: got /$RVAL/, expected /$TODO_UID/"
  exit 1;
fi
