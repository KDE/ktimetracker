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
    assert os.path.isfile('/tmp/testktimetracker1.ics')
