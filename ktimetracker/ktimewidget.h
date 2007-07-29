/*
 *     Copyright (C) 2000 by Jesper Perderson <blackie@kde.org>
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

#ifndef KARM_K_TIME_WIDGET_H
#define KARM_K_TIME_WIDGET_H

#include <QWidget>

class QLineEdit;

class KarmLineEdit;

/**
 * Widget used for entering minutes and seconds with validation.
 */

class KArmTimeWidget : public QWidget 
{
  public:
    explicit KArmTimeWidget( QWidget* parent = 0, const char* name = 0 );
    void setTime( int hour, int minute );
    long time() const;

  private:
    QLineEdit *_hourLE;
    KarmLineEdit *_minuteLE;
};

#endif // KARM_K_TIME_WIDGET_H
