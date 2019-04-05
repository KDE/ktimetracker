def test_delete(app_testfile):
    app_testfile.addTask('Task1')
    task_id = app_testfile.taskIdsFromName('Task1')[0]
    app_testfile.deleteTask(task_id)
    assert len(app_testfile.version()) > 0


def test_delete_parent(app_testfile):
    app_testfile.addTask('Task1')
    task_id = app_testfile.taskIdsFromName('Task1')[0]

    app_testfile.addSubTask('blah', task_id)
    app_testfile.deleteTask(task_id)

    assert len(app_testfile.version()) > 0
