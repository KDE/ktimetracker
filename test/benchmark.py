#!/usr/bin/env python3

import os
import signal
import time

from pydbus import SessionBus
from gi.repository import GLib


def waitDBusObject():
    bus = SessionBus()
    while True:
        try:
            return bus.get('org.kde.ktimetracker', '/KTimeTracker')
        except GLib.Error:
            time.sleep(0.5)


os.system('killall -q ktimetracker')

try:
    os.remove("/tmp/ktimetrackerbenchmark.ics")
except FileNotFoundError:
    pass

# start ktimetracker and make sure its dbus interface is ready
pid = os.spawnl(os.P_NOWAIT, "/usr/bin/ktimetracker", "ktimetracker", "/tmp/ktimetrackerbenchmark.ics")
app = waitDBusObject()

for n in range(2):
    # add 300 tasks
    for i in range(300):
        name = 'task%d-%d' % (n, i)
        app.addTask(name)

        ids = app.taskIdsFromName(name)
        assert len(ids) == 1

        app.startTimerFor(ids[0])

    app.saveAll()
    app.stopAllTimersDBUS()


os.kill(pid, signal.SIGTERM)

# Old comments from Bash-based benchmark:
#   This is a bash script for a ktimetracker benchmark - how fast is ktimetracker on your system ?
#   example: my 2.4GHz Quad-core X64 computer with 4GB RAM needs 50 seconds
