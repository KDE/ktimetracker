#include "task.h"
#include "print.h"
#include "klocale.h"
#include <qpainter.h>
#include <qpaintdevicemetrics.h>
#include <qdatetime.h>
#include <iostream>

MyPrinter::MyPrinter(const Karm *karm)
{
  _karm = karm;
}

void MyPrinter::Print()
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
    

		// Calculate the maximum width of the time fields
    // and calculate the totals
		totalTimeWidth = metrics.width(i18n("Total"));
    sessionTimeWidth = metrics.width(i18n("Session"));
    int totalTotal = 0;
    int sessionTotal = 0;
		for (QListViewItem *child = _karm->firstChild(); child;
				 child = child->nextSibling()) {
			Task *task = (Task *) child;

			totalTimeWidth = QMAX(totalTimeWidth, metrics.width(Karm::formatTime(task->totalTime())));
      totalTotal += task->totalTime();

      sessionTotal += task->sessionTime();
			sessionTimeWidth = QMAX(sessionTimeWidth, metrics.width(task->sessionTime()));
		}

    totalTimeWidth = QMAX(totalTimeWidth, metrics.width(Karm::formatTime(totalTotal)));
    sessionTimeWidth = QMAX(sessionTimeWidth, metrics.width(Karm::formatTime(sessionTotal)));
    nameFieldWidth = pageWidth - xMargin - totalTimeWidth - sessionTimeWidth - 2*5;
    

		// Print the header
		QFont origFont, newFont;
		origFont = painter.font();
		newFont = origFont;
		newFont.setPixelSize(origFont.pixelSize() * 1.5);
		painter.setFont(newFont);
		
		int height = metrics.height();
		QString now = QDateTime::currentDateTime().toString();
		
		painter.drawText(xMargin, yoff, pageWidth, height, QPainter::AlignCenter, 
										 "KArm - " + now);
		
		painter.setFont(origFont);
		yoff += height + 10;

    // Print the second header.
    PrintLine(i18n("Total"), i18n("Session"), i18n("Task Name"), painter);
		
    yoff += 4;
    painter.drawLine(xMargin, yoff, xMargin + pageWidth, yoff);
    yoff += 2;
    
		// Now print the actual content
		for (QListViewItem *child = _karm->firstChild(); child;
				 child = child->nextSibling()) {
			Task *task = (Task *) child;
			QString totalTime = Karm::formatTime(task->totalTime());
			QString sessionTime = Karm::formatTime(task->sessionTime());
			QString name = task->name();
			
      PrintLine(totalTime, sessionTime, name, painter);
    }
    

    yoff += 4;
    painter.drawLine(xMargin, yoff, xMargin + pageWidth, yoff);
    yoff += 2;
    
    // Print the Totals
    PrintLine(Karm::formatTime(totalTotal),
              Karm::formatTime(sessionTotal),
              QString(), painter);
    
    
	}
}

void MyPrinter::PrintLine(QString total, QString session, QString name, QPainter &painter)
{
  int xoff = xMargin;
  
  painter.drawText(xoff, yoff, totalTimeWidth, lineHeight, QPainter::AlignRight, total);
  xoff += totalTimeWidth+5;

  painter.drawText(xoff, yoff, sessionTimeWidth, lineHeight, QPainter::AlignRight, session);
  xoff += sessionTimeWidth+ 5;
  
  painter.drawText(xoff, yoff, nameFieldWidth, lineHeight, QPainter::AlignLeft, name);
			
  yoff += lineHeight;
  
  if (yoff + 2* lineHeight > pageHeight) {
    newPage();
    yoff = yMargin;
  }
}
