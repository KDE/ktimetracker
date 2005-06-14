#!/usr/bin/php
<?

// Description:
// This program starts karm and simulates keypresses to do a real life-test of karm.
// This program returns zero if all tests went ok, else an error code.
// You need a US or DE keyboard to run this.

// for those who do not know php:
// for a tutorial about php, check out www.usegroup.de
// for a reference about php, surf to www.php.net

// TODO
// prepare Windows-port

function createplannerexample()
{
$handle=fopen("/tmp/example.planner","w");
fwrite($handle,
'<?xml version="1.0"?>
<project name="" company="" manager="" phase="" project-start="20041101T000000Z" mrproject-version="2" calendar="1">
  <properties>
    <property name="cost" type="cost" owner="resource" label="Cost" description="standard cost for a resource"/>
  </properties>
  <phases/>
  <calendars>
    <day-types>
      <day-type id="0" name="Working" description="Ein Vorgabe-Arbeitstag"/>
      <day-type id="1" name="Nonworking" description="Ein Vorgabetag, an dem nicht gearbeitet wird"/>
      <day-type id="2" name="Basis verwenden" description="Use day from base calendar"/>
    </day-types>
    <calendar id="1" name="Vorgabe">
      <default-week mon="0" tue="0" wed="0" thu="0" fri="0" sat="1" sun="1"/>
      <overridden-day-types>
        <overridden-day-type id="0">
          <interval start="0800" end="1200"/>
          <interval start="1300" end="1700"/>
        </overridden-day-type>
      </overridden-day-types>
      <days/>
    </calendar>
  </calendars>
  <tasks>
    <task id="1" name="task 1" note="" work="28800" start="20041101T000000Z" end="20041101T170000Z" percent-complete="0" priority="0" type="normal" scheduling="fixed-work"/>
    <task id="2" name="task 2" note="" work="28800" start="20041101T000000Z" end="20041101T170000Z" percent-complete="0" priority="0" type="normal" scheduling="fixed-work">
      <task id="3" name="subtask 1-1" note="" work="28800" start="20041101T000000Z" end="20041101T170000Z" percent-complete="0" priority="0" type="normal" scheduling="fixed-work"/>
      <task id="4" name="subtask 1-2" note="" work="28800" start="20041101T000000Z" end="20041101T170000Z" percent-complete="0" priority="0" type="normal" scheduling="fixed-work"/>
    </task>
  </tasks>
  <resource-groups/>
  <resources/>
  <allocations/>
</project>    
');   
fclose($handle); 
};

function simkey($s)
// This function simulates keypresses that form the string $s, e.g. for $s==hallo, it simulates the keypress of h, then a, then l and so on.
// find a useful list of keycodes under /usr/include/X11/keysymdef.h
{
  for ($i=0; $i<strlen($s); $i++)
  {
    usleep(10000);            # this is just for the user to see what happens
    if ($s[$i]=="/") system("xte 'key KP_Divide'");
    else system("xte 'key ".$s[$i]."'");
    usleep(10000);            
  }
}

function keysim($s)
// remove everything that makes you have to think twice!!
{
  simkey($s);
}

function funkeysim($s, $count=1)
// same as keysim, but interprets $s as function key name to be used by xte and expects a $count to indicate how often key is to be pressed
{
  for ($i=1; $i<=$count; $i++) 
  {
    usleep(10000);            
    $rc=exec("xte 'key $s'");
    usleep(10000);            
  }
  return $rc;
}

// int main()
if ($argv[1]!="--batch")
{
  echo "\nThis is lifetest.php, a program to test karm by starting it and simulating keypresses.\n";
  echo "It is intended for developers who do changes to karm's sourcecode.\n";
  echo "Before publishing these changes, they should\n";
  echo "(a) resolve all conflicts with the latest karm sourcecode (cvs up)\n";
  echo "(b) make sure the code still builds (make)\n";
  echo "(c) run automated test routines like this (make check)\n\n";
  
  echo "This program simulates keypresses, so please leave the keyboard alone during the test. Please use a us or de keyboardlayout (setxkbmap us). This must be run in X environment.\n
  You must have XAutomation installed to run this.";
  system("xte -h 2&>/dev/null",$rc);
  if ($rc==0) echo " You have.\n";
  if ($rc==127) echo " You do not have, please get it from http://hoopajoo.net/projects/xautomation.html .\n";
  echo "This program will test karm by issueing karm, so, make sure, this calls the version you want to test (make install).\n\n";

  echo "This program will now stop unless you give the parameter --batch (confirming that you do not touch the keyboard)\n";   

  $err="";
  $exit=0;
}
else
{
  switch (funkeysim("Alt_L")) 
  {
    case 1: 
      $err.="this must be run in an X environment\n";
    break;
    case 127: 
      $err.="you do not have XAutomation installed, get it from http://hoopajoo.net/projects/xautomation.html\n";
    break;
  }
  // the following is the same as 'if file_exist(...) unlink(...)', but atomic
  @unlink ("/tmp/karmtest.ics");
  @unlink ("/tmp/example.planner");
  if ($err=="")
  { 
    // start and wait till mainwindow is up
    // the mouse can be in the way, so, move it out. This here even works with "focus strictly under mouse".
    system("xte 'mousemove 1 1'");
    echo "\nStarting karm";
    $process=popen("karm --geometry 200x100+0+0 /tmp/karmtest.ics >/dev/null 2>&1", 'w'); 
    $rc=1;
    while ($rc==1) system("dcop `dcop 2>/dev/null | grep karm` KarmDCOPIface version",$rc);
    echo "mainwindow is ready";
    sleep (1);
    
    funkeysim("Alt_L");
    
    funkeysim("Right",3);
    funkeysim("Down",2);
    funkeysim("Return");
    sleep (1);
    funkeysim("Down",2);
    funkeysim("Tab",5);
    simkey("/tmp/karmtest.ics");
    sleep (1);
    funkeysim("Return");
    sleep (1);
    funkeysim("Return");
    sleep (1);

    # add a new task
    funkeysim("Alt_L");
    funkeysim("Right",2);
    funkeysim("Down");
    sleep (1);
    funkeysim("Return");
    sleep (1);
    simkey("example 1");
    funkeysim("Return");
    sleep (1);
    
    echo "\nCreating a planner project file...";
    createplannerexample();
    
    # import planner project file
    funkeysim("Alt_L");
    funkeysim("Down",5);
    funkeysim("Right");
    funkeysim("Down");
    funkeysim("Return");
    sleep (2);
    keysim("/tmp/example.planner");
    sleep (1);
    funkeysim("Return");
    sleep (1);
    
    # export to CSV file
    funkeysim("Alt_L");
    funkeysim("Down",5);
    funkeysim("Right");
    funkeysim("Down",2);
    funkeysim("Return");
    sleep(1);
    keysim("/tmp/exporttest.csv");
    sleep(1);
    funkeysim("Tab",6);
    system ("xte 'keydown Alt_L'");
    system ("xte 'key m'");
    system ("xte 'keyup Alt_L'");
    sleep(1);
    funkeysim("Return");
    
    # send CTRL_Q
    sleep (2);
    echo "\nsending CTRL_Q...\n";
    system ("xte 'keydown Control_L'");
    system ("xte 'key Q'");
    system ("xte 'keyup Control_L'");
    
    $content=file_get_contents("/tmp/karmtest.ics");
    $lines=explode("\n",$content);
    if (!preg_match("/DTSTAMP:[0-9]{1,8}T[0-9]{1,6}Z/", $lines[4])) $err.="iCal file: wrong dtstamp";
    if ($lines[12]<>"SUMMARY:example 1") $err.="iCal file: wrong task example 1";
    if ($lines[16]<>"END:VTODO") $err.="iCal file: wrong end of vtodo";
    if ($lines[27]<>"SUMMARY:task 1") $err.="iCal file: wrong task task 1";
    if (!preg_match("/^UID:libkcal-[0-9]{1,8}.[0-9]{1,3}/", $lines[39])) $err.="iCal file: wrong uid";
    $content=file_get_contents("/tmp/exporttest.csv");
    $lines=explode("\n",$content);
    if (!preg_match("/\"example 1\",,0[,|.]00,0[,|.]00,0[,|.]00,0[,|.]00/", $lines[0])) $err.="csv export is wrong";
    pclose($process);
    @unlink ("/tmp/karmtest.ics");
    @unlink ("/tmp/example.planner");
    @unlink ("/tmp/exporttest.csv");
  }
}
  echo $err;
  if ($err!="") exit(1);
?>