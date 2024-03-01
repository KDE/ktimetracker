# SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>
#
# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL


def test_active_tasks(app_testfile):
    app_testfile.addTask('Task1')
    app_testfile.addTask('Task1')
    app_testfile.addTask('Task2')

    tasks1 = app_testfile.taskIdsFromName('Task1')
    tasks2 = app_testfile.taskIdsFromName('Task2')

    for task_id in tasks1 + tasks2:
        app_testfile.startTimerFor(task_id)
    assert app_testfile.activeTasks() == ['Task2', 'Task1', 'Task1']
    assert app_testfile.tasks() == ['Task2', 'Task1', 'Task1']

    for task_id in tasks2:
        app_testfile.stopTimerFor(task_id)
    assert app_testfile.activeTasks() == ['Task1', 'Task1']
    assert app_testfile.tasks() == ['Task2', 'Task1', 'Task1']

    assert app_testfile.isActive(tasks1[0]) == True
    assert app_testfile.isActive(tasks2[0]) == False

    assert app_testfile.isTaskNameActive('Task1') == True
    assert app_testfile.isTaskNameActive('Task2') == False


def test_start_stop_by_name(app_testfile):
    app_testfile.addTask('Task1')
    app_testfile.addTask('Task1')
    app_testfile.addTask('Task2')

    # startTimerForTaskName() starts only one task if there are duplicate names.
    assert app_testfile.startTimerForTaskName('Task1') == True
    assert app_testfile.startTimerForTaskName('Task2') == True
    assert app_testfile.activeTasks() == ['Task2', 'Task1']

    assert app_testfile.stopTimerForTaskName('Task2') == True
    assert app_testfile.activeTasks() == ['Task1']


def test_stop_all_times(app_testfile):
    app_testfile.addTask('Task1')
    app_testfile.addTask('Task1')
    app_testfile.addTask('Task2')

    for task_id in app_testfile.taskIdsFromName('Task1') + app_testfile.taskIdsFromName('Task2'):
        app_testfile.startTimerFor(task_id)
    assert len(app_testfile.activeTasks()) == 3

    app_testfile.stopAllTimersDBUS()
    assert app_testfile.activeTasks() == []
    assert len(app_testfile.tasks()) == 3


def test_set_percent_complete(app_testfile):
    app_testfile.addTask('Task1')
    task_id = app_testfile.taskIdsFromName('Task1')[0]

    app_testfile.startTimerFor(task_id)
    assert app_testfile.activeTasks() == ['Task1']

    app_testfile.setPercentComplete(task_id, 99)
    assert app_testfile.activeTasks() == ['Task1']

    # Task completion also stop the timer
    app_testfile.setPercentComplete(task_id, 100)
    assert app_testfile.activeTasks() == []
