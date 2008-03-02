<!-- WebTimeTracker
WebTimeTracker is a php web application that allows you to track how much time you spent on various tasks.
When you start a new task, you start a timer for it. At the end of the day, you can see how much time
you spent on various task.
WebTimeTracker uses ktimetracker as its engine, the php site starts ktimetracker on the server and 
communicates with it via dbus.

TODO: what happens if apache cannot start a gui ?
FIXME: where are my dbus bindings ?
TODO: you cannot use/create subtasks
FIXME: adding two tasks with the same name leads to unpredictable behavior
-->
<html>
<head>
<title>This is WebTimeTracker</title>
<head>
<body>
<h1>WebTimeTracker 0.01</h1>
<?php
if ($_GET['task'])
{ 
  exec("qdbus org.kde.ktimetracker /KTimeTracker addTask ".$_GET['task']);
}
if ($_GET['taskname'])
{
  exec("qdbus org.kde.ktimetracker /KTimeTracker taskIdsFromName ".$_GET['taskname'],$output,$errorput);
  if ($_GET['action'] == "start") exec("qdbus org.kde.ktimetracker /KTimeTracker startTimerFor ".$output[0]);
  if ($_GET['action'] == "stop") exec("qdbus org.kde.ktimetracker /KTimeTracker stopTimerFor ".$output[0]);
  if ($_GET['action'] == "delete") exec("qdbus org.kde.ktimetracker /KTimeTracker deleteTask ".$output[0]);
}
echo $HTTP_GET_VARS['task'];
echo $HTTP_POST_VARS['task'];
$handle=popen("/home/kde-devel/kde/bin/ktimetracker","r");
$errorlevel=1;
while ($errorlevel!=0)
{
  sleep (1);
  exec("qdbus org.kde.ktimetracker /KTimeTracker version",$output2,$errorlevel);
}
echo "Your ktimetracker is version $output2[0]<br />";
?>
<form action="index.php">
New Task:
<input name="task" type="text" size="30" maxlength="30">
<input type="submit" value="submit">
</form>
<table>
<?php
  exec("qdbus org.kde.ktimetracker /KTimeTracker tasks ",$tasknames,$errorlevel);
  for ($i=0; $i<sizeof($tasknames); $i++)
  {
    $output3=""; $output4="";
    exec("qdbus org.kde.ktimetracker /KTimeTracker taskIdsFromName ".$tasknames[$i],$output3);
    exec("qdbus org.kde.ktimetracker /KTimeTracker totalMinutesForTaskId ".$output3[0],$output4);    
    echo "<tr bgcolor=#FFEEEE><td>".$tasknames[$i]."</td><td>$output4[0]</td><td>";
    echo "<form action=\"index.php\">";
    echo "<input name=\"taskname\" type=\"hidden\" value=\"$tasknames[$i]\">";
    echo "<input name=\"action\" type=\"hidden\" value=\"start\">";
    echo "<input type=\"submit\" value=\"Start\">";
    echo "</form></td><td>";
    echo "<form action=\"index.php\">";
    echo "<input name=\"taskname\" type=\"hidden\" value=\"$tasknames[$i]\">";
    echo "<input name=\"action\" type=\"hidden\" value=\"stop\">";
    echo "<input type=\"submit\" value=\"Stop\">";
    echo "</form></td><td>";
    echo "<form action=\"index.php\">";
    echo "<input name=\"taskname\" type=\"hidden\" value=\"$tasknames[$i]\">";
    echo "<input name=\"action\" type=\"hidden\" value=\"delete\">";
    echo "<input type=\"submit\" value=\"Delete\">";
    echo "</form>";
    echo "</td></tr>";
  }
?>
</table>
</body>
</html>
