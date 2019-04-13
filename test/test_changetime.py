def test_change_time(app_testfile):
    app_testfile.addTask('Task1')
    task_id = app_testfile.taskIdsFromName('Task1')[0]
    retval = app_testfile.changeTime(task_id, 50)
    assert retval == 0
