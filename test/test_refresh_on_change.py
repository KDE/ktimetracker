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

import time
from datetime import datetime


def test_refresh_on_change(app_testfile):
    todo_name = 'todo name'
    todo_uid = 'abc-123'
    todo_time = datetime.now().strftime('%Y%m%dT%H%M%SZ')

    # make sure kdirstat can recognize the change in last change date
    time.sleep(1)

    f = open(app_testfile.path(), 'a')
    f.write(f"""BEGIN:VCALENDAR
PRODID:-//K Desktop Environment//NONSGML KTimeTracker Test Scripts//EN
VERSION:2.0

BEGIN:VTODO
DTSTAMP:{todo_time}
ORGANIZER;CN=Anonymous:MAILTO:nobody@nowhere
CREATED:{todo_time}
UID:{todo_uid}
SEQUENCE:0
LAST-MODIFIED:{todo_time}
SUMMARY:{todo_name}
CLASS:PUBLIC
PRIORITY:5
PERCENT-COMPLETE:0
END:VTODO

END:VCALENDAR
""")
    f.close()

    # wait so FAM and KDirWatcher tell karm and karm refreshes view
    time.sleep(2)

    task_id = app_testfile.taskIdsFromName(todo_name)[0]
    assert task_id == todo_uid
