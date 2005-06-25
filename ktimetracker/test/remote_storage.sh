#!/bin/sh

# Karm can read and write to an FTP server for storage file.

exec >>check.log 2>&1

PORT=8000
TESTFILE="http://localhost:$PORT/testkarm.ics"
TESTFILE_LOCAL="testkarm.ics"
TESTTODO="testtodo"

# Start with clean environment
# If runscripts sees output on stderr, it thinks script failed.
DCOPID=`dcop | grep karm 2>/dev/null`
if [ -n $DCOPID ]; then dcop $DCOPID KarmDCOPIface quit; fi;
if [ -e $TESTFILE_LOCAL ]; then rm $TESTFILE_LOCAL; fi

# if the file does not exist, kresources pops up a modal dialog box
# telling us that a file does not exist.

touch $TESTFILE_LOCAL

python __httpd.py $PORT &

sleep 2

HTTPD_PID=`ps -C "python __httpd.py" -o pid=`

karm $TESTFILE & 

sleep 2

DCOPID=`dcop | grep karm`

# karm does not write file until data is saved.

echo "DEBUG: dcop $DCOPID KarmDCOPIface addtodo \"$TESTTODO\""
dcop $DCOPID KarmDCOPIface addtodo "$TESTTODO"

sleep 1

RVAL=1
if [ -e $TESTFILE_LOCAL ]; then RVAL=0; fi

# clean up
if [ -n $DCOPID ]; then dcop $DCOPID KarmDCOPIface quit; fi;
if [ -e $TESTFILE_LOCAL ]; then rm $TESTFILE_LOCAL; fi
if [ -n $HTTPD_PID ]; then kill $HTTPD_PID; fi

# return 0 on success, 1 on failure
if [ $RVAL -eq 0 ]
then 
  echo "PASS $0"
  exit 0
else 
  echo "FAIL $0"
  exit 1
fi
