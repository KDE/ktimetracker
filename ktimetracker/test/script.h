/* This file is part of the KDE project
   Copyright (C) 2004 Mark Bucciarelli <mark@hubcapconsulting.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef _script_h_
#define _script_h_

//#include <qvariant.h>
#include <qobject.h>

class QDir;
class QProcess;
class QString;
class QStringList;

class Script : public QObject
{
  Q_OBJECT
public:
  Script( const QString& workingDirectory );
  virtual ~Script();
  void setProgram( const QString &arg );
  void addArgument( QString arg );
  void setTimeout( int seconds );
  int run();
private slots:
  void exit();
  void stderr();
  void stdout();
  void terminate();
private:
  QProcess *m_proc;
  bool m_stderr;
  int m_timeoutInSeconds;
  QString *program;   // name of the program to start, typically sh, php or perl
  QString arguments; // typically the name of a test script, e.g. webdav.sh
};

#endif // _script_h_
