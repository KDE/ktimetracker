# SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>
#
# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL


def test_same_name_tasks(app_testfile):
    app_testfile.addTask('Task1')
    app_testfile.addTask('Task1')
    task_ids = app_testfile.taskIdsFromName('Task1')
    assert len(task_ids) == 2
    assert len(task_ids[0]) == 36
    assert len(task_ids[1]) == 36
    assert task_ids[0] != task_ids[1]
