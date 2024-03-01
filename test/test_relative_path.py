# SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>
#
# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

import os


# Regression testing for https://bugs.kde.org/show_bug.cgi?id=94447
#
# Create files relative to current directory if no "/" prefix
# in file name given on command line.
def test_relative_path(app):
    try:
        os.remove('/tmp/testktimetracker1.ics')
    except FileNotFoundError:
        pass

    os.chdir('/tmp')
    app.run('testktimetracker1.ics')

    app.addTask('Task1')
    app.saveAll()
    assert os.path.isfile('/tmp/testktimetracker1.ics')
