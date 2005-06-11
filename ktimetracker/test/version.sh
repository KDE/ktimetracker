#!/bin/sh

# First test, just check version.

TESTFILE="/tmp/testkarm1.ics"
VERSION="1.6.0"

source __lib.sh

set_up

RVAL=`dcop $DCOPID KarmDCOPIface version 2>/dev/null`

tear_down

if [ "$RVAL" == "$VERSION" ]; then 
  echo "PASS $0"
  exit 0;
else 
  echo "FAIL $0: got /$RVAL/, expected /$VERSION/"
  exit 1;
fi
