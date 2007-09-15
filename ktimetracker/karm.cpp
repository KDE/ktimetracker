/*
 *     Copyright (C) 2006 by Thorsten Staerk <kde@staerk.de>
 *                   2007 the ktimetracker developers
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the
 *      Free Software Foundation, Inc.
 *      51 Franklin Street, Fifth Floor
 *      Boston, MA  02110-1301  USA.
 *
 */

/*
This program is here to remind the user that karm is now called ktimetracker. 
We cannot be sure the user is on a graphical display when starting this, 
so print out and display a messagebox.
We cannot be sure the user is aware the system wants to start karm, 
so say "someone, probably you, has called karm...".

Compile it like this:
g++ -I/home/kde-devel/qt-unstable/include/Qt -I/home/kde-devel/qt-unstable/include -I/home/kde-devel/qt-unstable/include/QtCore -I/home/kde-devel/qt-unstable/include/QtGui -I/home/kde-devel/kde/include -lQtCore_debug -L/home/kde-devel/qt-unstable/lib -L/home/kde-devel/kde/lib -lkdeui karm.cpp -o karm
*/

#include <QString>
#include <kapplication.h>
#include <kaboutdata.h>
#include <kmessagebox.h>
#include <kcmdlineargs.h>
#include <klocale.h>            // i18n
#include <iostream>

using namespace std;

int main (int argc, char *argv[])
{
  KAboutData aboutData( "KArmReminder", 0, ki18n("KArmReminder"),
      "1.0", ki18n("KArmReminder"), KAboutData::License_GPL,
      ki18n("(c) 2006") );
  KCmdLineArgs::init( argc, argv, &aboutData );
  KApplication khello;
  // outputting a string is again something that became complicated
  // Several languages need 8Bit, ascii() is not enough.
  cout << i18n("Someone, probably you, has called karm.\n").toLocal8Bit().data();
  cout << i18n("KArm has been renamed to KTimeTracker. This makes it easier to recognize.\nCompatibility advice: Do not give ktimetracker files to karm users. Using karm files with ktimetracker is possible.\n").toLocal8Bit().data();
  cout << i18n("Please learn to call ktimetracker as this reminder may be removed in the future.\n").toLocal8Bit().data();
  KMessageBox::information(0,i18n("Someone, probably you, has called karm. KArm has been renamed to KTimeTracker. This makes it easier to recognize. Compatibility advice: Do not give ktimetracker files to karm users. Using karm files with ktimetracker is possible. Please learn to call ktimetracker as this reminder may be removed in the future."),i18n("KArm is now ktimetracker"));
}
