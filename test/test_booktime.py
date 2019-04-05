def test_bad_date(app):
    app.addTask('Task1')
    task_id = app.taskIdsFromName('Task1')[0]
    retval = app.bookTime(task_id, 'notadate', 360)
    assert retval == 5


def test_bad_duration(app):
    app.addTask('Task1')
    task_id = app.taskIdsFromName('Task1')[0]
    retval = app.bookTime(task_id, '20050619', -5)
    assert retval == 7


def test_bad_time(app):
    app.addTask('Task1')
    task_id = app.taskIdsFromName('Task1')[0]
    retval = app.bookTime(task_id, '20050619Tabc', 360)
    assert retval == 5


def test_bad_uid(app):
    app.addTask('Task1')
    retval = app.bookTime('bad-uid', '20050619', 360)
    assert retval == 4


def test_valid(app):
    app.addTask('Task1')
    task_id = app.taskIdsFromName('Task1')[0]

    duration = 12
    retval = app.bookTime(task_id, '2005-06-19', duration)
    assert retval == 0

    retval = app.totalMinutesForTaskId(task_id)
    assert retval == duration
