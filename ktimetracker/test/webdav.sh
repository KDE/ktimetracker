#!/bin/sh
# This is a test script for ktimetracker. To make sure ktimetracker run correctly, run this script. Most probably, you will 
# only run this script if you modified the source code of ktimetracker.

# Add a todo to an iCal file stored on a webdav server.

# Start webdav server
perl __webdav.pl &
sleep 2
WEBDAV_PID=`ps -C "perl __webdav.pl" -o pid=`

# Start ktimetracker and make sure its dbus interface is ready
testfile="http://localhost:4242/testkarm.ics"
TESTFILE_LOCAL="/tmp/testkarm.ics"
TESTTODO="testtodo"
SKIP_TESTFILE_DELETE=true
ktimetracker $testfile & while ! qdbus org.kde.ktimetracker /KTimeTracker version; do i=5; done
# Need this or karm complains there is no file
rm -f $TESTFILE_LOCAL
touch $TESTFILE_LOCAL
#wait till download is ready
sleep 3

# add a todo
qdbus org.kde.ktimetracker /KTimeTracker org.kde.ktimetracker.ktimetracker.addTask "$TESTTODO"
qdbus org.kde.ktimetracker /KTimeTracker org.kde.ktimetracker.ktimetracker.save
sleep 1

if grep $TESTTODO $TESTFILE_LOCAL
	then RVAL=0
	else RVAL=1
fi

#if [ -e $TESTFILE_LOCAL ]; then rm $TESTFILE_LOCAL; fi
if [ -n $WEBDAV_PID ]; then kill $WEBDAV_PID; fi

# return 0 on success, 1 on failure
if [ $RVAL -eq 0 ]
then 
  echo "PASS $0"
  exit 0
else 
  echo "FAIL $0"
  exit 1
fi
