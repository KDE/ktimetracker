#!/bin/sh

TESTFILE="/tmp/testkarm1.ics"

source __lib.sh

set_up

dcop $DCOPID KarmDCOPIface addTask Task1 2>/dev/null
TASKID=`dcop $DCOPID KarmDCOPIface taskIdFromName Task1 2>/dev/null`
RVAL=`dcop $DCOPID KarmDCOPIface bookTime $TASKID 20050619Tabc 360 2>/dev/null`

tear_down

EXPECTED=5
if [ "$RVAL" == "$EXPECTED" ]; then 
  echo "PASS $0"
  exit 0;
else 
  echo "FAIL $0: got /$RVAL/, expected /$EXPECTED/"
  exit 1;
fi
