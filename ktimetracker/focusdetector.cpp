/*
* Copyright (C) 2007 René Mérou
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
* Free Software Foundation, Inc.
* 51 Franklin Street, Fifth Floor
* Boston, MA  02110-1301  USA.
*
*  at short Logic that gets and stores tasks from focused windows.
*  at author René Mérou <ochominutosdearco at gmail.com>
*/
#include <QProcess>
#include "focusdetector.h"

#include <qdatetime.h>
#include <qmessagebox.h>
#include <qtimer.h>

#include <kdialog.h>
#include <kglobal.h>
#include <klocale.h> // i18n
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#ifdef Q_WS_X11
#include <QX11Info>
#endif
 QString lastWindow="";
FocusDetector::FocusDetector(int periodFocus)
{
  _timer = new QTimer(this);
  connect(_timer, SIGNAL(timeout()), this, SLOT(check()));
  _timer->start(1000*periodFocus);
}

void FocusDetector::check()
{
  QProcess focusQuestion;
  QString cmd="./getactivewindowtitle";
  QString sysanswer;
  focusQuestion.setProcessChannelMode(QProcess::MergedChannels);
  focusQuestion.start( cmd );

  if (!focusQuestion.waitForFinished())
    kDebug() << "Command " << cmd << " failed:" << focusQuestion.errorString();
  else
    sysanswer=focusQuestion.readAll();
  kDebug() << "sysanswer=" << sysanswer << endl;
  if (lastWindow!=sysanswer) 
  {
    lastWindow=sysanswer;
    kDebug() << "NEW WINDOW WITH FOCUS; Sending signal:"  << endl;
    emit (newFocus(sysanswer));
  }
}

void FocusDetector::setPeriodFocus(int periodFocus)
{
  _periodFocus = periodFocus;
}

void FocusDetector::startFocusDetection()
{
  if (!_timer->isActive())
    _timer->start(periodInterval);
}

void FocusDetector::stopFocusDetection()
{
  if (_timer->isActive())
    _timer->stop();
}

#include "focusdetector.moc" 
