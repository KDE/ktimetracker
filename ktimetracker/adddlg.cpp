/*
 *   karm
 *   This file only: Copyright (C) 1999  Espen Sand, espensa@online.no
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
 * $Id:
 * $Log:
 */


#include <qlabel.h>
#include <qlineedit.h>
#include <qlayout.h>

#include <kapp.h>
#include <klocale.h>

#include "adddlg.h"

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

  text = i18n("Accumulated time\n(in minutes)");
  label = new QLabel( text, page, "time" );
  topLayout->addWidget( label, 1, 0 );

  mTaskTime = new QLineEdit( page, "lineedit" );
  mTaskTime->setMinimumWidth(fontMetrics().maxWidth()*15);
  topLayout->addWidget( mTaskTime, 1, 1 );
}


void AddTaskDialog::setTask( const QString &name, long time )
{
  mTaskName->setText( name );
  mTaskTime->setText( QString().setNum( time ) );
}


QString AddTaskDialog::taskName( void ) const
{ 
  return( mTaskName->text() ); 
}


long AddTaskDialog::taskTime( void ) const
{ 
  return( atol( mTaskTime->text().ascii()) ); 
}


void AddTaskDialog::slotOk( void )
{
  emit finished( true );
}


void AddTaskDialog::slotCancel( void )
{
  emit finished( false );
}
