# Copyright (c) 2019 Alexander Potashev <aspotashev@gmail.com>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License or (at your option) version 3 or any later version
# accepted by the membership of KDE e.V. (or its successor approved
# by the membership of KDE e.V.), which shall act as a proxy
# defined in Section 14 of version 3 of the license.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

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
