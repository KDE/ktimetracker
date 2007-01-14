
# Expects karm test file in $TESTFILE 
# Returns dcop id in $DCOP_ID
function set_up()
{
  DCOPID=`dcop 2>/dev/null | grep karm`

  if [ -n "$DCOPID" ]; then dcop $DCOPID KarmDCOPIface quit; fi;

  if [ "x$SKIP_TESTFILE_DELETE" != "xtrue" ]; then
	  if [ -e "$TESTFILE" ]; then rm $TESTFILE; fi
  fi

  #echo "__lib.sh - starting karm with $TESTFILE"
  karm "$TESTFILE" & 

  # Make sure karm is up and running
  limit=10
  idx=0
  DCOPID=""
  while [ "$idx" -lt "$limit"  ]
  do
    #echo "__lib.sh: dcop 2>/dev/null | grep karm"
    DCOPID=`dcop 2>/dev/null | grep karm`
    if [ -n "$DCOPID" ]
    then
      break
    else
      let "idx += 1"
    fi
    sleep 1
  done

  # It's not enough to get the dcop id, as this is available almost
  # immediately.  We need to make sure karm (and fam) is done loading data.
  limit=20
  idx=0
  KARM_VERSION=""
  while [ "$idx" -lt "$limit"  ]
  do
    #echo "__lib.sh: dcop $DCOPID KarmDCOPIface version 2>/dev/null"
    KARM_VERSION=`dcop $DCOPID KarmDCOPIface version 2>/dev/null`
    if [ -n "$KARM_VERSION" ]
    then
      break
    else
      let "idx += 1"
    fi
    sleep 1
  done

  if [ "x$DCOPID" = x ]
  then
    echo "__lib.sh set_up error: could not start karm--no dcop id."
    exit 1
  else
    echo "__lib.sh: DCOPID = $DCOPID, KARM_VERSION = $KARM_VERSION"
  fi

  if [ "x$KARM_VERSION" = x ]
  then
    echo "__lib.sh set_up error: karm did not return a version string."
    exit 1
  fi
}

function test_func()
{
  echo "Yep, that works."
}

function tear_down()
{
  if [ -n "$DCOPID" ]; then dcop "$DCOPID" KarmDCOPIface quit; fi;

  if [ "x$SKIP_TESTFILE_DELETE" != "xtrue" ]; then
		if [ -e "$TESTFILE" ]; then rm "$TESTFILE"; fi
	fi
}
