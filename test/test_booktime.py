# SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>
#
# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL


def test_bad_date(app_testfile):
    app_testfile.addTask('Task1')
    task_id = app_testfile.taskIdsFromName('Task1')[0]
    retval = app_testfile.bookTime(task_id, 'notadate', 360)
    assert retval == 5


def test_bad_duration(app_testfile):
    app_testfile.addTask('Task1')
    task_id = app_testfile.taskIdsFromName('Task1')[0]
    retval = app_testfile.bookTime(task_id, '20050619', -5)
    assert retval == 7


def test_bad_time(app_testfile):
    app_testfile.addTask('Task1')
    task_id = app_testfile.taskIdsFromName('Task1')[0]
    retval = app_testfile.bookTime(task_id, '20050619Tabc', 360)
    assert retval == 5


def test_bad_uid(app_testfile):
    app_testfile.addTask('Task1')
    retval = app_testfile.bookTime('bad-uid', '20050619', 360)
    assert retval == 4


def test_valid(app_testfile):
    app_testfile.addTask('Task1')
    task_id = app_testfile.taskIdsFromName('Task1')[0]

    duration = 12
    assert app_testfile.bookTime(task_id, '2005-06-19', duration) == 0

    assert app_testfile.totalMinutesForTaskId(task_id) == duration
