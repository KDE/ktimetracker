/*
    This file is part of KTimeTracker.
    Copyright (c) 2003 Tobias Koenig <tokoe@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

    As a special exception, permission is given to link this program
    with any edition of Qt, and distribute the resulting executable,
    without including the source code for Qt in the source distribution.
*/

#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTabWidget>

#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QBoxLayout>

#include <kconfig.h>
#include <kdebug.h>
#include <kdialog.h>
#include <klocale.h>
#include <kmessagebox.h>

#include "ktimetrackerconfigwidget.h"

KTimeTrackerConfigWidget::KTimeTrackerConfigWidget( QWidget *parent, const char *name )
  : QWidget( parent )
{

  QTabWidget *tabWidget = new QTabWidget( this );

}

void KTimeTrackerConfigWidget::restoreSettings()
{
  bool blocked = signalsBlocked();
  blockSignals( true );

  mLocationMapURL->lineEdit()->setCursorPosition( 0 );

  KConfig _config("ktimetrackerrc", KConfig::NoGlobals);
  KConfigGroup config(&_config, "General" );
  mTradeAsFamilyName->setChecked( config.readEntry( "TradeAsFamilyName", true ) );
  mLimitContactDisplay->setChecked( config.readEntry( "LimitContactDisplay", true ) );

  blockSignals( blocked );

  emit changed( false );
}

void KTimeTrackerConfigWidget::saveSettings()
{

  KConfig _config("ktimetrackerrc", KConfig::NoGlobals);
  KConfigGroup config(&_config, "General" );
  config.writeEntry( "TradeAsFamilyName", mTradeAsFamilyName->isChecked() );
  config.writeEntry( "LimitContactDisplay", mLimitContactDisplay->isChecked() );

  emit changed( false );
}

void KTimeTrackerConfigWidget::defaults()
{
  mNameParsing->setChecked( true );
  mViewsSingleClickBox->setChecked( false );
  mEditorCombo->setCurrentIndex( 0 );
  mLimitContactDisplay->setChecked( true );

  emit changed( true );
}

void KTimeTrackerConfigWidget::modified()
{
  emit changed( true );
}

#include "ktimetrackerconfigwidget.moc"
