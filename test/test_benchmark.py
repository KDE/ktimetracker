# SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>
#
# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL


# Old comments from Bash-based benchmark:
#   This is a bash script for a ktimetracker benchmark - how fast is ktimetracker on your system ?
#   example: my 2.4GHz Quad-core X64 computer with 4GB RAM needs 50 seconds
def run(app):
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


def test_benchmark(benchmark, app_testfile):
    benchmark.pedantic(run, args=(app_testfile,))
