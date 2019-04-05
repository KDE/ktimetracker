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
