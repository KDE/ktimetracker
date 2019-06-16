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
