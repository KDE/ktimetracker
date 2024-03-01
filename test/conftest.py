# SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>
#
# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

import pytest

from test.fixture.app import AppHelper


@pytest.fixture(scope='function')
def app():
    app_helper = AppHelper()
    yield app_helper
    app_helper.stop()


@pytest.fixture(scope='function')
def app_testfile():
    app_helper = AppHelper()
    app_helper.run('/tmp/ktimetrackertest.ics')
    yield app_helper
    app_helper.stop()
