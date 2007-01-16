/*
 *     Copyright (C) 2007 the ktimetracker developers
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

#ifndef KARM_PRINT_H
#define KARM_PRINT_H

#undef Color // X11 headers
#undef GrayScale // X11 headers
#include <kprinter.h>

class QPainter;
class QString;

class Task;
class TaskView;

/**
 * Provide printing capabilities.
 */

class MyPrinter : public KPrinter
{
  public:
    MyPrinter( const TaskView *taskView );
    void print();
    void printLine( QString total, QString session, QString name, QPainter &,
                    int );
    void printTask( Task *task, QPainter &, int level );  
    int calculateReqNameWidth( Task *task, QFontMetrics &metrics,
                               int level);
  
  private:
    const TaskView *_taskView;

    int xMargin, yMargin;
    int yoff;
    int timeWidth;
    int sessionTimeWidth;
    int nameFieldWidth;
    int lineHeight;
    int pageHeight;  
};

#endif // KARM_PRINT_H
