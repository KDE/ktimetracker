/* This file is part of the KDE project
   Copyright (C) 2002 Anders Lund <anders@alweb.dk>
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
#include <qdir.h>
#include <qprocess.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qtimer.h>

#include <kdebug.h>

#include "script.h"

/*
        n.b. Do not use kDebug statements in this file.

        With qt-copy 3_3_BRANCH, they cause a Valgrind error.
        Ref: KDE bug #95237.
*/

// Wait for terminate() attempt to return before using kill()
// kill() doesn't let script interpreter try to clean up.
const int NICE_KILL_TIMEOUT_IN_SECS = 5;

Script::Script( const QString& workingDirectory )
{
  m_status = 0;
  m_stderr = false;
  m_timeoutInSeconds = 5;

  m_proc   = new QProcess( this );
  m_proc->setWorkingDirectory( workingDirectory );

  program = new QString();
  arguments = QString();

  connect ( m_proc, SIGNAL( readyReadStdout() ), 
            this  , SLOT  ( stdout() ) 
            );
  connect ( m_proc, SIGNAL( readyReadStderr() ), 
            this  , SLOT  ( stderr() ) 
            );
  connect ( m_proc, SIGNAL( processExited() ), 
            this  , SLOT  ( exit() ) 
            );
}

Script::~Script()
{
  delete m_proc;
  m_proc = 0;
}

void Script::setProgram ( const QString &arg )
{
  kDebug() << "Entering setProgram" << endl;
  program=new QString(arg);
  kDebug() << "Leaving setProgram" << endl;
}

void Script::addArgument( QString arg )
{
  kDebug() << "Entering addArgument" << endl;
  kDebug() << "argument to be added is " << arg << endl;
  if (arguments!=QString()) arguments.append(" ");
  arguments.append(arg);
  kDebug() << "arguments are now " << arguments << endl;
  kDebug() << "Leaving addArgument" << endl;
}

void Script::setTimeout( int seconds )
{
  if ( seconds <= 0 ) return;
  m_timeoutInSeconds = seconds;
}

int Script::run()
{
  kDebug() << "Entering Script::run" << endl;
  const QString program1=program->left(program->length()); 
  const QStringList qsl=arguments.split(" ");
  kDebug() << "Program is " << program1 << endl;
  kDebug() << "Arguments are " << qsl << endl;
  int err;
  m_proc->execute(program1, qsl);
  // This didn't work.  But Ctrl-C does.  :P
  //QTimer::singleShot( m_timeoutInSeconds * 1000, m_proc, SLOT( kill() ) );
  //while ( ! m_proc->normalExit() );
  return m_status;
}

void Script::terminate()
{
  // These both trigger processExited, so exit() will run.
  m_proc->terminate();
  QTimer::singleShot( NICE_KILL_TIMEOUT_IN_SECS*1000, m_proc, SLOT( kill() ) );
}

void Script::exit()
{
  m_status = m_proc->exitStatus();
  delete m_proc;
  m_proc = 0;
}

void Script::stderr()
{
  // Treat any output to std err as a script failure
  m_status = 1;
  QString data = QString( m_proc->readAllStandardError() );
  m_stderr= true;
}

void Script::stdout()
{
  QString data = QString( m_proc->readAllStandardOutput() );
}

#include "script.moc"
