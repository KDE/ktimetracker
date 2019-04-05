import time
import os

from pydbus import SessionBus
from gi.repository import GLib


class AppHelper(object):
    def __init__(self):
        os.system('killall -q ktimetracker')

        try:
            os.remove("/tmp/ktimetrackerbenchmark.ics")
        except FileNotFoundError:
            pass

        # start ktimetracker and make sure its dbus interface is ready
        self.pid = os.spawnl(os.P_NOWAIT, "/usr/bin/ktimetracker", "ktimetracker", "/tmp/ktimetrackerbenchmark.ics")
        self.app = self.__wait_dbus_object()

    @staticmethod
    def __wait_dbus_object():
        bus = SessionBus()
        while True:
            try:
                return bus.get('org.kde.ktimetracker', '/KTimeTracker')
            except GLib.Error:
                time.sleep(0.5)
