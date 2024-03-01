# SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>
#
# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL


def test_delete(app_testfile):
    app_testfile.addTask('Task1')
    task_id = app_testfile.taskIdsFromName('Task1')[0]
    app_testfile.deleteTask(task_id)
    assert app_testfile.tasks() == []
    assert len(app_testfile.version()) > 0


def test_delete_parent(app_testfile):
    app_testfile.addTask('Task1')
    task_id = app_testfile.taskIdsFromName('Task1')[0]

    app_testfile.addSubTask('blah', task_id)
    app_testfile.deleteTask(task_id)

    # Should delete both tasks: parent and child
    assert app_testfile.tasks() == []

    assert len(app_testfile.version()) > 0
