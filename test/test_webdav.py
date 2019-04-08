import threading
import time
import os
from http.server import HTTPServer

from pywebdav.server.fileauth import DAVAuthHandler
from pywebdav.server.fshandler import FilesystemHandler


def webdav_server():
    class DummyConfigDAV:
        def getboolean(self, name):
            return False

    class DummyConfig:
        DAV = DummyConfigDAV()

    host = 'localhost'
    port = 4242

    handler = DAVAuthHandler
    handler._config = DummyConfig()

    handler.DO_AUTH = False
    handler.IFACE_CLASS = FilesystemHandler('/tmp', 'http://%s:%d/' % (host, port))
    handler.IFACE_CLASS.baseurl = ''

    httpd = HTTPServer((host, port), handler)

    httpd.serve_forever()


def test_webdav(app):
    remote_path = 'http://localhost:4242/test_ktimetracker_webdav.ics'
    local_path = '/tmp/test_ktimetracker_webdav.ics'

    try:
        os.remove(local_path)
    except FileNotFoundError:
        pass

    # Start webdav server
    daemon = threading.Thread(target=webdav_server)
    daemon.setDaemon(True)
    daemon.start()
    time.sleep(2)

    # Start KTimeTracker
    app.run(remote_path)

    # wait till download is ready
    time.sleep(1)

    # add a todo
    todo_name = 'testtodo'
    app.addTask(todo_name)
    app.saveAll()
    time.sleep(1)

    with open(local_path, 'r') as content_file:
        assert todo_name in content_file.read()
