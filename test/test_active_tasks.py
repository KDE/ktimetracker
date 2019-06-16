# Copyright (c) 2019 Alexander Potashev <aspotashev@gmail.com>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License or (at your option) version 3 or any later version
# accepted by the membership of KDE e.V. (or its successor approved
# by the membership of KDE e.V.), which shall act as a proxy
# defined in Section 14 of version 3 of the license.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.


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
