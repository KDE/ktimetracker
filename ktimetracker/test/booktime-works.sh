#!/bin/sh

TESTFILE="/tmp/testkarm1.ics"

source __lib.sh

set_up

DURATION=12
dcop $DCOPID KarmDCOPIface addTask Task1 2>/dev/null
TASKID=`dcop $DCOPID KarmDCOPIface taskIdFromName Task1 2>/dev/null`
RVAL=`dcop $DCOPID KarmDCOPIface bookTime $TASKID 2005-06-19 $DURATION 2>/dev/null`

if [ "x$RVAL" != "x0" ]; then 
  echo "FAIL $0: got /$RVAL/, expected /$EXPECTED/"
  exit 1;
fi

RVAL=`dcop $DCOPID KarmDCOPIface totalMinutesForTaskId $TASKID 2>/dev/null`

SKIP_TESTFILE_DELETE=true
tear_down

if [ "x$RVAL" == "x$DURATION" ]; then 
  echo "PASS $0"
  exit 0;
else 
  echo "FAIL $0: got /$RVAL/, expected /$DURATION/"
  exit 1;
fi
