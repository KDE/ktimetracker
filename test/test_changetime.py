# SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>
#
# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL


def test_change_time(app_testfile):
    app_testfile.addTask('Task1')
    task_id = app_testfile.taskIdsFromName('Task1')[0]
    retval = app_testfile.changeTime(task_id, 50)
    assert retval == 0

    assert app_testfile.totalMinutesForTaskId(task_id) == 50
