/*
 *   karm
 *   This file only: Copyright (C) 1999  Espen Sand, espensa@online.no
 *   Modifications (see CVS log) Copyright (C) 2000 Klarälvdalens
 *   Datakonsult AB, <kalle@dalheimer.de>, Jesper Pedersen <blackie@ifad.dk>
 *
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
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */


/* 
 * $Id:$
 * $Log:$
 */
#include <qlabel.h>
#include <qlineedit.h>
#include <qlayout.h>
#include <qregexp.h>

#include <kapp.h>
#include <klocale.h>

#include "adddlg.h"
#include "karm.h"

AddTaskDialog::AddTaskDialog( QWidget *parent, const char *name, bool modal )
  :KDialogBase( parent, name, modal, "task", Ok|Cancel, Ok, true )
{
  QWidget *page = new QWidget( this ); 
  setMainWidget(page);

  QGridLayout *topLayout = new QGridLayout( page, 2, 2, 0, spacingHint() );

  QString text = i18n("Task name");
  QLabel *label = new QLabel( text, page, "name" );
  topLayout->addWidget( label, 0, 0 );
  
  mTaskName = new QLineEdit( page, "lineedit" );
  mTaskName->setMinimumWidth(fontMetrics().maxWidth()*15);
  topLayout->addWidget( mTaskName, 0, 1 );

  text = i18n("Accumulated time\n(HH:MM [+|- HH:MM])");
  label = new QLabel( text, page, "time" );
  topLayout->addWidget( label, 1, 0 );

  mValidator = new TimeValidator(this);
  mTaskTime = new QLineEdit( page, "lineedit" );
  mTaskTime->setMinimumWidth(fontMetrics().maxWidth()*15);
  mTaskTime->setValidator(mValidator);
  topLayout->addWidget( mTaskTime, 1, 1 );
}


void AddTaskDialog::setTask( const QString &name, long minutes )
{
  mTaskName->setText( name );
  mTaskTime->setText(Karm::formatTime(minutes));
}


QString AddTaskDialog::taskName( void ) const
{ 
  return( mTaskName->text() ); 
}


long AddTaskDialog::taskTime( void ) const
{ 
  QString time = mTaskTime->text();
  long res;
  (void) mValidator->extractTime(time, &res);
  return res;
}


void AddTaskDialog::slotOk( void )
{
  emit finished( true );
}


void AddTaskDialog::slotCancel( void )
{
  emit finished( false );
}

enum QValidator::State TimeValidator::validate(QString &str, int &) const
{
  long dummy;
  if (extractTime(str, &dummy)) {
	return Acceptable;
  }
  else {
	return Invalid;
  }
	
}

bool TimeValidator::extractTime(QString time, long *res) const
{
  QString part;
  long minutes = 0;
  int pm=1; // Was the last a plus or minus
  int nextPm=1;
  bool ok1(true), ok2(true);

  while (!time.isEmpty()) {
	pm = nextPm;
		
	int plusIndex = time.find("+");
	int minusIndex = time.find("-");
	if ( (plusIndex != -1 && minusIndex != -1 && plusIndex < minusIndex) ||
		 minusIndex == -1) {
	  if (plusIndex != -1) {
		part = time.left(plusIndex);
		time = time.remove(0,plusIndex+1);
		nextPm = 1;
	  }
	  else {
		part = time;
		time = "";
	  }
	}
	else {
	  if (minusIndex != -1) {
		part = time.left(minusIndex);
		time = time.remove(0,minusIndex+1);
		nextPm = -1;
	  }
	  else {
		part = time;
		time = "";
	  }
	}

	int colonIndex = part.find(":");
	if (colonIndex != -1) {
	  QString hour = part.left(colonIndex);
	  QString min = part.remove(0,colonIndex+1);
	  if (hour.stripWhiteSpace() != "") 
		minutes += pm * 60 * hour.toLong(&ok1);
	  if (min.stripWhiteSpace() != "")
		minutes += pm * min.toLong(&ok2);
	}
	else {
	  if (part.stripWhiteSpace() != "")
		minutes += pm * part.toLong(&ok1);
	}
	if (!ok1 || !ok2) {
	  return false;
	}

		
  }
  if (minutes < 0)
	minutes = 0;
	
  *res = minutes;
  return true;
}

	
