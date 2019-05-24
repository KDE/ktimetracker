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

# Old comments from Bash-based benchmark:
#   This is a bash script for a ktimetracker benchmark - how fast is ktimetracker on your system ?
#   example: my 2.4GHz Quad-core X64 computer with 4GB RAM needs 50 seconds
def run(app):
    for n in range(2):
        # add 300 tasks
        for i in range(300):
            name = 'task%d-%03d' % (n, i)
            app.addTask(name)

            ids = app.taskIdsFromName(name)
            assert len(ids) == 1

            app.startTimerFor(ids[0])

        app.saveAll()
        app.stopAllTimersDBUS()


def test_benchmark(benchmark, app_testfile):
    benchmark.pedantic(run, args=(app_testfile,))
