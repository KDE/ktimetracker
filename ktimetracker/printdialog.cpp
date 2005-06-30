/*
 *   This file only:
 *     Copyright (C) 2003  Mark Bucciarelli <mark@hubcapconsutling.com>
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
 *      59 Temple Place - Suite 330
 *      Boston, MA  02111-1307  USA.
 *
 */

#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qpixmap.h>
#include <qpushbutton.h>
#include <qstring.h>
#include <qwidget.h>
#include <qwhatsthis.h>

#include <kiconloader.h>
#include <klocale.h>            // i18n
#include <kwinmodule.h>

#include "printdialog.h"
#include <libkdepim/kdateedit.h>


PrintDialog::PrintDialog()
  : KDialogBase(0, "PrintDialog", true, i18n("Print Dialog"), Ok|Cancel,
      Ok, true )
{
  QWidget *page = new QWidget( this );
  setMainWidget(page);
  int year, month;

  QVBoxLayout *layout = new QVBoxLayout(page, KDialog::spacingHint());
  layout->addSpacing(10);
  layout->addStretch(1);

  // Date Range
  QGroupBox *rangeGroup = new QGroupBox(1, Horizontal, i18n("Date Range"),
      page);
  layout->addWidget(rangeGroup);

  QWidget *rangeWidget = new QWidget(rangeGroup);
  QHBoxLayout *rangeLayout = new QHBoxLayout(rangeWidget, 0, spacingHint());

  rangeLayout->addWidget(new QLabel(i18n("From:"), rangeWidget));
  _from = new KDateEdit(rangeWidget);

  // Default from date to beginning of the month
  year = QDate::currentDate().year();
  month = QDate::currentDate().month();
  _from->setDate(QDate(year, month, 1));
  rangeLayout->addWidget(_from);
  rangeLayout->addWidget(new QLabel(i18n("To:"), rangeWidget));
  _to = new KDateEdit(rangeWidget);
  rangeLayout->addWidget(_to);

  layout->addSpacing(10);
  layout->addStretch(1);

  _allTasks = new QComboBox( page );
  _allTasks->insertItem( i18n( "Selected Task" ) );
  _allTasks->insertItem( i18n( "All Tasks" ) );
  layout->addWidget( _allTasks );

  _perWeek = new QCheckBox( i18n( "Summarize per week" ), page );
  layout->addWidget( _perWeek );
  _totalsOnly = new QCheckBox( i18n( "Totals Only" ), page );
  layout->addWidget( _totalsOnly );

  layout->addSpacing(10);
  layout->addStretch(1);
}

QDate PrintDialog::from() const
{
  return _from->date();
}

QDate PrintDialog::to() const
{
  return _to->date();
}

bool PrintDialog::perWeek() const
{
  return _perWeek->isChecked();
}

bool PrintDialog::allTasks() const
{
  return _allTasks->currentItem() == 1;
}

bool PrintDialog::totalsOnly() const
{
  return _totalsOnly->isChecked();
}

#include "printdialog.moc"
