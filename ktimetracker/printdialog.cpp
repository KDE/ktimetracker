/*
 *     Copyright (C) 2003 by Mark Bucciarelli <mark@hubcapconsutling.com>
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

#include "printdialog.h"

#include <QCheckBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

#include <KComboBox>
#include <KLocale>            // i18n

#include <libkdepim/kdateedit.h>

PrintDialog::PrintDialog()
  : KDialog()
{
  setObjectName( "PrintDialog" );
  QWidget *page = new QWidget( this );
  setMainWidget(page);
  int year, month;

  QVBoxLayout *layout = new QVBoxLayout(page);
  layout->setMargin( KDialog::marginHint() );
  layout->setSpacing(KDialog::spacingHint());
  layout->addSpacing(10);
  layout->addStretch(1);

  // Date Range
  QGroupBox *rangeGroup = new QGroupBox(i18n("Date Range"), page);
  layout->addWidget(rangeGroup);

  QHBoxLayout *rangeLayout = new QHBoxLayout;
  rangeLayout->setSpacing( KDialog::spacingHint() );
  rangeLayout->setMargin( KDialog::marginHint() );

  QLabel *label = new QLabel( i18n("From:"), rangeGroup );
  rangeLayout->addWidget( label );
  _from = new KPIM::KDateEdit(rangeGroup);
  label->setBuddy( _from );

  // Default from date to beginning of the month
  year = QDate::currentDate().year();
  month = QDate::currentDate().month();
  _from->setDate(QDate(year, month, 1));
  rangeLayout->addWidget(_from);
  label = new QLabel( i18n("To:"), rangeGroup );
  rangeLayout->addWidget( label );
  _to = new KPIM::KDateEdit(rangeGroup);
  label->setBuddy( _to );
  rangeLayout->addWidget(_to);
  rangeGroup->setLayout( rangeLayout );

  layout->addSpacing(10);
  layout->addStretch(1);

  _allTasks = new KComboBox( page );
  _allTasks->addItem( i18n( "Selected Task" ) );
  _allTasks->addItem( i18n( "All Tasks" ) );
  layout->addWidget( _allTasks );

  _perWeek = new QCheckBox( i18n( "Summarize per week" ), page );
  layout->addWidget( _perWeek );
  _totalsOnly = new QCheckBox( i18n( "Totals only" ), page );
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
  return _allTasks->currentIndex() == 1;
}

bool PrintDialog::totalsOnly() const
{
  return _totalsOnly->isChecked();
}

#include "printdialog.moc"
