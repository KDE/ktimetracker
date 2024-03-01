# SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>
#
# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

import os
import tempfile


def test_import(app_testfile):
    app_testfile.importPlannerFile('invalid')
    assert app_testfile.tasks() == []

    path = os.path.join(os.path.dirname(__file__), '../src/tests/data/kitchen.planner')
    app_testfile.importPlannerFile(path)
    assert len(app_testfile.tasks()) == 21


def test_export(app_testfile):
    app_testfile.addTask('Task1')
    app_testfile.addTask('Task2')

    task_id = app_testfile.taskIdsFromName('Task1')[0]
    app_testfile.changeTime(task_id, 135)

    fd, tempfile_path = tempfile.mkstemp(text=True)
    app_testfile.exportCSVFile(tempfile_path, '', '', 0, False, True, '*', '<>')

    with open(fd, closefd=True) as f:
        lines = f.readlines()
        assert len(lines) == 2
        assert lines[0] == '<>Task2<>*0:00*0:00*0:00*0:00\n'
        assert lines[1] == '<>Task1<>*2:15*2:15*2:15*2:15\n'
