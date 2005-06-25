#!/bin/sh

# Add a todo to an iCal file stored on a webdav server.

exec >>check.log 2>&1

source __lib.sh

# Start webdav server
perl __webdav.pl &
sleep 2
WEBDAV_PID=`ps -C "perl __webdav.pl" -o pid=`

# Start karm
TESTFILE="http://localhost:4242/testkarm.ics"
TESTFILE_LOCAL="/tmp/testkarm.ics"
TESTTODO="testtodo"
SKIP_TESTFILE_DELETE=true
# Need this or karm complains there is no file
touch $TESTFILE_LOCAL
set_up

# add a todo
dcop $DCOPID KarmDCOPIface addtodo "$TESTTODO"

sleep 1

if grep $TESTTODO $TESTFILE_LOCAL
	then RVAL=0
	else RVAL=1
fi

# clean up
tear_down
if [ -e $TESTFILE_LOCAL ]; then rm $TESTFILE_LOCAL; fi
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
