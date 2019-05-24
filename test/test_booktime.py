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
    retval = app_testfile.bookTime(task_id, '2005-06-19', duration)
    assert retval == 0

    retval = app_testfile.totalMinutesForTaskId(task_id)
    assert retval == duration
