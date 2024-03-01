# SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>
#
# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

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
