#include <iostream>
#include "task.h"
#include "print.h"
#include "klocale.h"
#include <qpainter.h>
#include <qpaintdevicemetrics.h>
#include <qdatetime.h>

const int levelIndent = 10;

MyPrinter::MyPrinter(const Karm *karm)
{
  _karm = karm;
}

void MyPrinter::print()
{
	if (setup()) {
		// setup
		QPainter painter(this);
		QPaintDeviceMetrics deviceMetrics(this);
		QFontMetrics metrics = painter.fontMetrics();
		pageHeight = deviceMetrics.height();
		int pageWidth = deviceMetrics.width();
    xMargin = margins().width();
    yMargin = margins().height();
    yoff = yMargin;
    lineHeight = metrics.height();
    

		// Calculate the totals
    // Note the totals are only calculated at the top most levels, as the
    // totals are increased together with its children.
    int totalTotal = 0;
    int sessionTotal = 0;
		for (QListViewItem *child = _karm->firstChild(); child;
				 child = child->nextSibling()) {
			Task *task = (Task *) child;

      totalTotal += task->totalTime();
      sessionTotal += task->sessionTime();
		}

    // Calculate the needed width for each of the fields
    totalTimeWidth = QMAX(metrics.width(i18n("Total")),
                          metrics.width(Karm::formatTime(totalTotal)));
    sessionTimeWidth = QMAX(metrics.width(i18n("Session")),
                            metrics.width(Karm::formatTime(sessionTotal)));

    nameFieldWidth = pageWidth - xMargin - totalTimeWidth - sessionTimeWidth - 2*5;
    
    int maxReqNameFieldWidth=0;
    for (QListViewItem *child = _karm->firstChild(); child;
         child = child->nextSibling()) {
      int width = calculateReqNameWidth(child, metrics, 0);
      maxReqNameFieldWidth = QMAX(maxReqNameFieldWidth, width);
    }
    nameFieldWidth = QMIN(nameFieldWidth, maxReqNameFieldWidth);

    int realPageWidth = nameFieldWidth + totalTimeWidth + sessionTimeWidth + 2*5;

		// Print the header
		QFont origFont, newFont;
		origFont = painter.font();
		newFont = origFont;
		newFont.setPixelSize(origFont.pixelSize() * 1.5);
		painter.setFont(newFont);
		
		int height = metrics.height();
		QString now = QDateTime::currentDateTime().toString();
		
		painter.drawText(xMargin, yoff, pageWidth, height,
				 QPainter::AlignCenter, 
				 i18n("KArm - %1").arg(now));
		
		painter.setFont(origFont);
		yoff += height + 10;

    // Print the second header.
    printLine(i18n("Total"), i18n("Session"), i18n("Task Name"), painter, 0);
		
    yoff += 4;
    painter.drawLine(xMargin, yoff, xMargin + realPageWidth, yoff);
    yoff += 2;
    
    // Now print the actual content
    for (QListViewItem *child = _karm->firstChild(); child;
       child = child->nextSibling()) {
      printTask(child, painter, 0);
    }
    

    yoff += 4;
    painter.drawLine(xMargin, yoff, xMargin + realPageWidth, yoff);
    yoff += 2;
    
    // Print the Totals
    printLine(Karm::formatTime(totalTotal),
              Karm::formatTime(sessionTotal),
              QString(), painter, 0);
    
    
	}
}

int MyPrinter::calculateReqNameWidth(QListViewItem *item,
                                     QFontMetrics &metrics,
                                     int level) 
{
  Task *task = (Task *) item;
  int width = metrics.width(QString::fromLatin1(task->name())) + level * levelIndent;

  for (QListViewItem *child = item->firstChild(); child;
       child = child->nextSibling()) {
    int childWidth = calculateReqNameWidth(child, metrics, level+1);
    width = QMAX(width, childWidth);
  }
  return width;
}

void MyPrinter::printTask(QListViewItem *item, QPainter &painter, int level)
{
  Task *task = (Task *) item;
  QString totalTime = Karm::formatTime(task->totalTime());
  QString sessionTime = Karm::formatTime(task->sessionTime());
  QString name = QString::fromLatin1(task->name());
  printLine(totalTime, sessionTime, name, painter, level);

  for (QListViewItem *child = item->firstChild(); child;
       child = child->nextSibling()) {
    printTask(child, painter, level+1);
  }      
}

void MyPrinter::printLine(QString total, QString session, QString name, 
                          QPainter &painter, int level)
{
  int xoff = xMargin + 10 * level;
  
  painter.drawText(xoff, yoff, nameFieldWidth, lineHeight, QPainter::AlignLeft, name);
  xoff = xMargin + nameFieldWidth;
  
  painter.drawText(xoff, yoff, sessionTimeWidth, lineHeight, QPainter::AlignRight, session);
  xoff += sessionTimeWidth+ 5;
  
  painter.drawText(xoff, yoff, totalTimeWidth, lineHeight, QPainter::AlignRight, total);
  xoff += totalTimeWidth+5;

  yoff += lineHeight;
  
  if (yoff + 2* lineHeight > pageHeight) {
    newPage();
    yoff = yMargin;
  }
}
