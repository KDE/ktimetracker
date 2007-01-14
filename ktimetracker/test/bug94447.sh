#!/bin/sh

# Create files relative to current directory if no "/" prefix
# in file name given on command line

exec >>check.log 2>&1

TESTFILE="testkarm.ics"
TESTTODO="testtodo"

source __lib.sh 

set_up

# make karm create the file.
dcop $DCOPID KarmDCOPIface addtodo "$TESTTODO"

RVAL=1
if [ -e $TESTFILE ]; then RVAL=0; fi

tear_down

if [ $RVAL -eq 0 ]
then 
  echo "PASS $0"
  exit 0
else 
  echo "FAIL $0"
  exit 1
fi
