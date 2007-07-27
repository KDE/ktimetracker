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

#ifndef KARM_FOCUS_DETECTOR_H
#define KARM_FOCUS_DETECTOR_H

#include <QDateTime>
#include <QObject>
#include "config-karm.h" // HAVE_LIBXSS

class QTimer;

const int periodInterval = 1000;

/**
 * Keep track of what window has the focus.
 */

class FocusDetector :public QObject
{
Q_OBJECT

public:
  /**
     Initializes the time period
      at param periodFocus minutes before every focus detection.
  **/
  FocusDetector(int periodFocus);

  /**
     Sets the period of time before every focus search.
      at param periodFocus period of time in minutes
  **/
  void setPeriodFocus(int periodFocus);

  /**
     Starts detecting focus
  **/
  void startFocusDetection();

  /**
      Stops detecting focus.
  **/
  void stopFocusDetection();

signals:
    void newFocus(QString);

protected slots:
  void check();

private:
  int _periodFocus;
  QTimer *_timer;
  QDateTime start; // when the periodFocus restarted

};

#endif // KARM_FOCUS_DETECTOR_H 

