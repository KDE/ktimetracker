/*
 *     Copyright (C) 2000 by Jesper Pedersen <blackie@kde.org>
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

#include "print.h"

#include <QDateTime>
#include <QPainter>
#include <QtGui/QPrintDialog>

#include <KGlobal>
#include <KLocale>            // i18n

#include "karmutility.h"        // formatTime()
#include "task.h"
#include "taskview.h"

const int levelIndent = 10;

MyPrinter::MyPrinter(const TaskView *taskView)
{
  _taskView = taskView;
}

void MyPrinter::print()
{

  QPrintDialog printDialog(this, 0L);
  // FIXME: make a better caption for the printingdialog
  printDialog.setWindowTitle(i18n("Print Times"));
  if (printDialog.exec()) {
    // setup
    QPainter painter(this);
    QFontMetrics metrics = painter.fontMetrics();
    pageHeight = this->height();
    int pageWidth = this->width();
    xMargin = paperRect().left() - pageRect().left();
    yMargin = paperRect().top() - pageRect().top();
    yoff = yMargin;
    lineHeight = metrics.height();
    
    // Calculate the totals
    // Note the totals are only calculated at the top most levels, as the
    // totals are increased together with its children.
    int totalTotal = 0;
    int sessionTotal = 0;
    for ( int i = 0; i < _taskView->topLevelItemCount(); ++i ) {
      Task *task = static_cast< Task* >( _taskView->topLevelItem( i ) );
      totalTotal += task->totalTime();
      sessionTotal += task->totalSessionTime();
    }

    // Calculate the needed width for each of the fields
    timeWidth = qMax(metrics.width(i18nc( "total time of all tasks", "Total" )),
                     metrics.width(formatTime(totalTotal)));
    sessionTimeWidth = qMax(metrics.width(i18n("Session")),
                            metrics.width(formatTime(sessionTotal)));

    nameFieldWidth = pageWidth - xMargin - timeWidth - sessionTimeWidth - 2*5;
    
    int maxReqNameFieldWidth= metrics.width(i18n("Task Name "));
    
    for ( int i = 0; i < _taskView->topLevelItemCount(); ++i ) {
      Task *task = static_cast< Task* >( _taskView->topLevelItem( i ) );
      int width = calculateReqNameWidth(task, metrics, 0);
      maxReqNameFieldWidth = qMax(maxReqNameFieldWidth, width);
    }
    nameFieldWidth = qMin(nameFieldWidth, maxReqNameFieldWidth);

    int realPageWidth = nameFieldWidth + timeWidth + sessionTimeWidth + 2*5;

    // Print the header
    QFont origFont, newFont;
    origFont = painter.font();
    newFont = origFont;
    newFont.setPixelSize( static_cast<int>(origFont.pixelSize() * 1.5) );
    painter.setFont(newFont);
    
    int height = metrics.height();
    QString now = KGlobal::locale()->formatDateTime(QDateTime::currentDateTime());
    
    painter.drawText(xMargin, yoff, pageWidth, height,
         Qt::AlignCenter, 
         i18n("KArm - %1", now));
    
    painter.setFont(origFont);
    yoff += height + 10;

    // Print the second header.
    printLine(i18nc( "total time of all tasks", "Total" ), i18n("Session"), i18n("Task Name"), painter, 0);
    
    yoff += 4;
    painter.drawLine(xMargin, yoff, xMargin + realPageWidth, yoff);
    yoff += 2;
    
    // Now print the actual content
    for ( int i = 0; i < _taskView->topLevelItemCount(); ++i ) {
      Task *task = static_cast< Task* >( _taskView->topLevelItem( i ) );
      printTask(task, painter, 0);
    }

    yoff += 4;
    painter.drawLine(xMargin, yoff, xMargin + realPageWidth, yoff);
    yoff += 2;
    
    // Print the Totals
    printLine( formatTime( totalTotal ),
               formatTime( sessionTotal ),
               QString(), painter, 0);
  }
}

int MyPrinter::calculateReqNameWidth( Task* task,
                                      QFontMetrics &metrics,
                                      int level) 
{
  int width = metrics.width(task->name()) + level * levelIndent;

  for ( int i = 0; i < task->childCount(); ++i ) {
    Task *subTask = static_cast< Task* >( task->child( i ) );
    int subTaskWidth = calculateReqNameWidth(subTask, metrics, level+1);
    width = qMax(width, subTaskWidth);
  }
  return width;
}

void MyPrinter::printTask(Task *task, QPainter &painter, int level)
{
  QString time = formatTime(task->totalTime());
  QString sessionTime = formatTime(task->totalSessionTime());
  QString name = task->name();
  printLine(time, sessionTime, name, painter, level);

  for ( int i = 0; i < task->childCount(); ++i ) {
    Task *subTask = static_cast< Task* >( task->child( i ) );
    printTask(subTask, painter, level+1);
  }      
}

void MyPrinter::printLine( const QString &total, const QString &session, 
                           const QString &name, QPainter &painter, int level )
{
  int xoff = xMargin + 10 * level;

  painter.drawText( xoff, yoff, nameFieldWidth, lineHeight,
                    Qt::AlignLeft, name);
  xoff = xMargin + nameFieldWidth;

  painter.drawText( xoff, yoff, sessionTimeWidth, lineHeight,
                    Qt::AlignRight, session);
  xoff += sessionTimeWidth+ 5;

  painter.drawText( xoff, yoff, timeWidth, lineHeight,
                    Qt::AlignRight, total);
  xoff += timeWidth+5;

  yoff += lineHeight;

  if ( yoff + 2 * lineHeight > pageHeight ) {
    newPage();
    yoff = yMargin;
  }
}
