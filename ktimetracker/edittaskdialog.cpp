/*
 *     Copyright (C) 1999 by Espen Sand <espensa@online.no>
 *                   2000 by Klarälvdalens Datakonsult AB <kalle@dalheimer.de>
 *                   2000 by Jesper Pedersen <blackie@kde.org>
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

#include "edittaskdialog.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include <KComboBox>
#include <KDebug>
#include <KLineEdit>
#include <KLocale>            // i18n
#include <KWindowSystem>

#include "ktimewidget.h"

EditTaskDialog::EditTaskDialog( QWidget *parent, const QString &caption, 
                                bool editDlg, DesktopList* desktopList)
  : KDialog( parent ),
    origTime( 0 ), origSession( 0 )
{
  setWindowTitle( caption );
  setObjectName( "EditTaskDialog" );
  QWidget *page = new QWidget( this ); 
  setMainWidget( page );

  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->setMargin( 0 );
  mainLayout->setSpacing( KDialog::spacingHint() );

  /* Task name */
  QWidget *taskNameWidget = new QWidget( page );
  QHBoxLayout *layout = new QHBoxLayout;
  layout->setMargin( 0 );
  layout->setSpacing( KDialog::spacingHint() );
  QLabel *label = new QLabel( i18n( "Task &name:" ), taskNameWidget );
  _name = new KLineEdit( taskNameWidget );
  label->setBuddy( _name );
  layout->addWidget( label );
  layout->addWidget( _name );
  taskNameWidget->setLayout( layout );

  /* The "Edit Absolut" radio button */
  QWidget *timeWidget = new QWidget( page );
  QGridLayout *gridLayout = new QGridLayout;
  gridLayout->setMargin( 0 );
  gridLayout->setSpacing( 0 );
  gridLayout->setColumnMinimumWidth( 0, 20 );
  gridLayout->setColumnMinimumWidth( 2, KDialog::spacingHint() );
  _absoluteRB = new QRadioButton( i18n( "Edit &absolute" ), timeWidget );
  connect( _absoluteRB, SIGNAL( clicked() ),
           this, SLOT( slotAbsolutePressed() ) );
  _timeLA = new QLabel( i18n( "&Time:" ), timeWidget );
  _timeTW = new KArmTimeWidget( timeWidget );
  _timeLA->setBuddy( _timeTW );
  _sessionLA = new QLabel( i18n( "&Session time:" ), timeWidget );
  _sessionTW = new KArmTimeWidget( timeWidget );
  _sessionLA->setBuddy( _sessionTW );

  /* The "Edit relative" radio button */
  _relativeRB = new QRadioButton( i18n( "Edit &relative (apply to both time and"
      " session time)" ), timeWidget );
  connect( _relativeRB, SIGNAL( clicked() ), 
           this, SLOT( slotRelativePressed() ) );
  _operator = new KComboBox( timeWidget );
  _operator->addItem( QString::fromLatin1( "+" ) );
  _operator->addItem( QString::fromLatin1( "-" ) );

  _diffTW = new KArmTimeWidget( timeWidget );
  gridLayout->addWidget(_absoluteRB, 0, 0, 1, 4 );
  gridLayout->addWidget( _timeLA, 1, 1 );
  gridLayout->addWidget( _timeTW, 1, 3 );
  gridLayout->addWidget( _sessionLA, 2, 1 );
  gridLayout->addWidget( _sessionTW, 2, 3 );
  gridLayout->addWidget( _relativeRB, 3, 0, 1, 4 );
  gridLayout->addWidget( _operator, 4, 1, 1, 2 );
  gridLayout->addWidget( _diffTW, 4, 3 );
  timeWidget->setLayout( gridLayout );

  /* Auto Tracking */
#ifdef Q_WS_X11
  desktopCount = KWindowSystem::numberOfDesktops();
#else
#ifdef __GNUC__
#warning non-X11 support missing
#endif
  desktopCount = 1;
#endif

  if ( desktopList && (desktopList->size() > 0) )
  {
    DesktopList::iterator rit = desktopList->begin();
    while ( *rit < desktopCount && rit!=desktopList->end() )
    {
      ++rit;
    }
    desktopList->erase( rit, desktopList->end() );
  }

  int lines = (int)(desktopCount/2);
  if (lines*2 != desktopCount) lines++; 

  QGroupBox *groupBox = new QGroupBox( i18n( "A&uto Tracking" ), page );
  groupBox->setCheckable( true );
  groupBox->setChecked( false );
  gridLayout = new QGridLayout;
  gridLayout->setMargin( KDialog::marginHint() );
  gridLayout->setSpacing( KDialog::spacingHint() );
  label = new QLabel( i18n( "In Desktop" ) );
  gridLayout->addWidget( label, 0, 0, 1, lines );

  for ( int i = 0; i < desktopCount; ++i )
  {
    QCheckBox *tmpBx = new QCheckBox( groupBox );
    tmpBx->setObjectName( QString::number( i ).toLatin1() );
    _deskBox.append( tmpBx );
#ifdef Q_WS_X11
    tmpBx->setText( KWindowSystem::desktopName( i + 1 ) );
#endif
    tmpBx->setChecked( false );

    gridLayout->addWidget( tmpBx, i / lines + 1, i % lines );
  }

  groupBox->setLayout( gridLayout );

  // check specified Desktop Check Boxes
  bool enableDesktops = false;

  if ( desktopList && ( desktopList->size() > 0 ) )
  {
    DesktopList::iterator it = desktopList->begin();
    while ( it != desktopList->end() )
    {
      _deskBox[*it]->setChecked( true );
      ++it;
    }
    enableDesktops = true;
  }
  // if some desktops were specified, then enable the parent box
  groupBox->setChecked( enableDesktops );

  connect(groupBox, SIGNAL( clicked( bool ) ),
          this, SLOT( slotAutoTrackingPressed( bool ) ) );

  mainLayout->addWidget( taskNameWidget );
  mainLayout->addWidget( timeWidget );
  mainLayout->addWidget( groupBox );
  page->setLayout( mainLayout );

  if ( editDlg )
  {
    // This is an edit dialog.
    _operator->setFocus();
  }
  else
  {
    // This is an initial dialog
    _name->setFocus();
  }

  slotRelativePressed();

  // Whats this help.
  _name->setWhatsThis(
                   i18n( "Enter the name of the task here. "
                         "You can choose it freely."));
  _absoluteRB->setWhatsThis(
                   i18n( "Use this option to set the time spent on this task "
                         "to an absolute value.\n\nFor example, if you have "
                         "worked exactly four hours on this task during the current "
                         "session, you would set the Session time to 4 hr." ) );
  _relativeRB->setWhatsThis(
                   i18n( "Use this option to change the time spent on this task "
                         "relative to its current value.\n\nFor example, if you worked "
                         "on this task for one hour without the timer running, you "
                         "would add 1 hr." ) );
  _timeTW->setWhatsThis(
                   i18n( "This is the time the task has been "
                         "running since all times were reset."));
  _sessionTW->setWhatsThis(
                   i18n( "This is the time the task has been running this "
                         "session."));
  _diffTW->setWhatsThis( 
                   i18n( "Specify how much time to add or subtract "
                         "to the overall and session time"));
  groupBox->setWhatsThis( 
                   i18n( "Use this option to automatically start the timer "
                         "on this task when you switch to the specified desktop(s). "
                         "Select the desktop(s) that will automatically start the "
                         "timer on this task." ) );
}

void EditTaskDialog::slotAbsolutePressed()
{
  _relativeRB->setChecked( false );
  _absoluteRB->setChecked( true );

  _operator->setEnabled( false );
  _diffTW->setEnabled( false );

  _timeLA->setEnabled( true );
  _sessionLA->setEnabled( true );
  _timeTW->setEnabled( true );
  _sessionTW->setEnabled( true );
}

void EditTaskDialog::slotRelativePressed()
{
  _relativeRB->setChecked( true );
  _absoluteRB->setChecked( false );

  _operator->setEnabled( true );
  _diffTW->setEnabled( true );

  _timeLA->setEnabled( false );
  _sessionLA->setEnabled( false );
  _timeTW->setEnabled( false );
  _sessionTW->setEnabled( false );
}

void EditTaskDialog::slotAutoTrackingPressed( bool checked ) 
{
  if (!checked)  // uncheck all desktop boxes
    for ( int i = 0; i < desktopCount; ++i ) 
      _deskBox[i]->setChecked( false );
}

void EditTaskDialog::setTask( const QString &name, long time, long session )
{
  _name->setText( name );

  _timeTW->setTime( time / 60, time % 60 );
  _sessionTW->setTime( session / 60, session % 60 );
  origTime = time;
  origSession = session;
}

QString EditTaskDialog::taskName() const
{
  return( _name->text() );
}

void EditTaskDialog::status(long *time, long *timeDiff, long *session,
                            long *sessionDiff, DesktopList *desktopList) const
{
  if ( _absoluteRB->isChecked() )
  {
    *time = _timeTW->time();
    *session = _sessionTW->time();
  } else
  {
    int diff = _diffTW->time();
    if ( _operator->currentIndex() == 1)
    {
      diff = -diff;
    }
    *time = origTime + diff;
    *session = origSession + diff;
  }

  *timeDiff = *time - origTime;
  *sessionDiff = *session - origSession;

  for ( int i = 0; i < _deskBox.count(); i++ )
  {
    if ( _deskBox[i]->isChecked() )
      desktopList->append( i );
  }
}

#include "edittaskdialog.moc"
