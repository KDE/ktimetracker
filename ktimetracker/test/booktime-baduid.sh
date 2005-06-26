#!/bin/sh

# First test, just check version.

TESTFILE="/tmp/testkarm1.ics"

source __lib.sh

set_up

RVAL=`dcop $DCOPID KarmDCOPIface bookTime bad-uid  20050619 360 2>/dev/null`

tear_down

EXPECTED=4
if [ "$RVAL" == "$EXPECTED" ]; then 
  echo "PASS $0"
  exit 0;
else 
  echo "FAIL $0: got /$RVAL/, expected /$EXPECTED/"
  exit 1;
fi
