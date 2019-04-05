import time
import os
import signal

from pydbus import SessionBus
from gi.repository import GLib


class AppHelper(object):
    def __init__(self):
        os.system('killall -q ktimetracker')

    def run(self, path):
        try:
            os.remove(path)
        except FileNotFoundError:
            pass

        # start ktimetracker and make sure its dbus interface is ready
        self.__pid = os.spawnl(os.P_NOWAIT, "/usr/bin/ktimetracker", "ktimetracker", path)
        self.app = self.__wait_dbus_object()

    def stop(self):
        os.kill(self.__pid, signal.SIGTERM)

    def __getattr__(self, name):
        return getattr(self.app, name)

    @staticmethod
    def __wait_dbus_object():
        bus = SessionBus()
        while True:
            try:
                return bus.get('org.kde.ktimetracker', '/KTimeTracker')
            except GLib.Error:
                time.sleep(0.5)
