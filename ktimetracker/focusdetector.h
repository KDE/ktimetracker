/*
 *     Copyright (C) 2007 by René Mérou <ochominutosdearco@gmail.com>
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

#ifndef KTIMETRACKER_FOCUS_DETECTOR_H
#define KTIMETRACKER_FOCUS_DETECTOR_H

#include <kwindowsystem.h>
#include <QObject>

#include <config-ktimetracker.h> // HAVE_LIBXSS

class QTimer;

/**
  Keep track of what window has the focus.
 */
class FocusDetector : public QObject
{
Q_OBJECT

public:
  /**
    Initializes the time period
   */
  FocusDetector();

public slots:
  void slotfocuschanged();

Q_SIGNALS:
  void newFocus( const QString & );
};

#endif // KTIMETRACKER_FOCUS_DETECTOR_H
