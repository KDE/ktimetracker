#!/usr/bin/env python3

import os
import signal

from test.fixture.app import AppHelper

# start ktimetracker and make sure its dbus interface is ready
appHelper = AppHelper()
app = appHelper.app

for n in range(2):
    # add 300 tasks
    for i in range(300):
        name = 'task%d-%03d' % (n, i)
        app.addTask(name)

        ids = app.taskIdsFromName(name)
        assert len(ids) == 1

        app.startTimerFor(ids[0])

    app.saveAll()
    app.stopAllTimersDBUS()


os.kill(appHelper.pid, signal.SIGTERM)

# Old comments from Bash-based benchmark:
#   This is a bash script for a ktimetracker benchmark - how fast is ktimetracker on your system ?
#   example: my 2.4GHz Quad-core X64 computer with 4GB RAM needs 50 seconds
