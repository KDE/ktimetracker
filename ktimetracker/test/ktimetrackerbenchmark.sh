#!/bin/bash

# This is a bash script for a ktimetracker benchmark - how fast is ktimetracker on your system ?
# example: my 2.4GHz Quad-core X64 computer with 4GB RAM needs 20 seconds

# preparation
killall ktimetracker
rm /tmp/ktimetrackerbenchmark.ics 2>&1 | grep -v "no such file or directory"

time(

  # start ktimetracker and make sure its dbus interface is ready
  ktimetracker /tmp/ktimetrackerbenchmark.ics & while ! qdbus org.kde.ktimetracker /KTimeTracker version; do i=5; done                                                      
  
  # add 300 tasks
  for i in $(seq 1 1 300); do qdbus org.kde.ktimetracker /KTimeTracker addTask task$i; done 
)