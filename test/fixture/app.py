import time
import os
import signal

from pydbus import SessionBus
from gi.repository import GLib


class AppHelper(object):
    def __init__(self):
        os.system('killall -q ktimetracker')

    def run(self, path):
        self.__path = path

        try:
            os.remove(path)
        except FileNotFoundError:
            pass

        # start ktimetracker and make sure its dbus interface is ready
        self.__pid = os.spawnlp(os.P_NOWAIT, "ktimetracker", "ktimetracker", path)
        self.app = self.__wait_dbus_object()

    def path(self):
        return self.__path

    def stop(self):
        os.kill(self.__pid, signal.SIGTERM)

    def __getattr__(self, name):
        return getattr(self.app, name)

    def __wait_dbus_object(self):
        bus = SessionBus()
        while True:
            try:
                os.waitpid(self.__pid, os.P_NOWAIT)
            except OSError:
                raise RuntimeError('KTimeTracker process stopped unexpectedly')

            try:
                return bus.get('org.kde.ktimetracker', '/KTimeTracker')
            except GLib.Error:
                time.sleep(0.5)
