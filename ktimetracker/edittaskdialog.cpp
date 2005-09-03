/*
 *   karm
 *   This file only: Copyright (C) 1999  Espen Sand, espensa@online.no
 *   Modifications (see CVS log) Copyright (C) 2000 Klarälvdalens
 *   Datakonsult AB <kalle@dalheimer.de>, Jesper Pedersen <blackie@kde.org>
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
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */
#include <q3buttongroup.h>
#include <qcombobox.h>
#include <q3groupbox.h>
#include <q3hbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qradiobutton.h>
#include <qsizepolicy.h>
#include <qstring.h>
#include <qwidget.h>
#include <q3whatsthis.h>
//Added by qt3to4:
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>

#include <klocale.h>            // i18n
#include <kwinmodule.h>

#include "edittaskdialog.h"
#include "ktimewidget.h"
#include "kdebug.h"

EditTaskDialog::EditTaskDialog( QString caption, bool editDlg,
                                DesktopList* desktopList)
  : KDialogBase(0, "EditTaskDialog", true, caption, Ok|Cancel, Ok, true ),
    origTime( 0 ), origSession( 0 )
{
  QWidget *page = new QWidget( this ); 
  setMainWidget(page);
  KWinModule kwinmodule(0, KWinModule::INFO_DESKTOP);

  QVBoxLayout *lay1 = new QVBoxLayout(page);
  
  QHBoxLayout *lay2 = new QHBoxLayout();
  lay1->addLayout(lay2);
  
  // The name of the widget
  QLabel *label = new QLabel( i18n("Task &name:"), page, "name" );
  lay2->addWidget( label );
  lay2->addSpacing(5);
  
  
  _name = new QLineEdit( page, "lineedit" );
  
  _name->setMinimumWidth(fontMetrics().maxWidth()*15);
  lay2->addWidget( _name );
  label->setBuddy( _name );


  // The "Edit Absolut" radio button
  lay1->addSpacing(10);lay1->addStretch(1); 
  _absoluteRB = new QRadioButton( i18n( "Edit &absolute" ), page,
                                  "_absoluteRB" );
  lay1->addWidget( _absoluteRB );
  connect( _absoluteRB, SIGNAL( clicked() ), this, SLOT( slotAbsolutePressed() ) );
  

  // Absolute times
  QHBoxLayout *lay5 = new QHBoxLayout();
  lay1->addLayout(lay5);
  lay5->addSpacing(20);
  QGridLayout *lay3 = new QGridLayout( 2, 2, -1, "lay3" );
  lay5->addLayout(lay3);
  
  _sessionLA = new QLabel( i18n("&Session time: "), page, "session time" );

  // Time
  _timeLA = new QLabel( i18n("&Time:"), page, "time" );
  lay3->addWidget( _timeLA, 0, 0 );
  _timeLA->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, 
                                         (QSizePolicy::SizeType)0, 
                                         0, 
                                         0, 
                                         _timeLA->sizePolicy().hasHeightForWidth()) );

  // Based on measuring pixels in a screenshot, it looks like the fontmetrics
  // call includes the ampersand when calculating the width.  To be sure
  // things will line up (no matter what language or widget style), set all
  // three date entry label controls to the same width.
  _timeLA->setMinimumWidth( fontMetrics().width( _sessionLA->text() ) );

  _timeTW = new KArmTimeWidget( page, "_timeTW" );
  lay3->addWidget( _timeTW, 0, 1 );
  _timeLA->setBuddy( _timeTW );
  

  // Session
  lay3->addWidget( _sessionLA, 1, 0 );

  _sessionTW = new KArmTimeWidget( page, "_sessionTW" );
  lay3->addWidget( _sessionTW, 1, 1 );
  _sessionLA->setBuddy( _sessionTW );
  _sessionLA->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, 
                                         (QSizePolicy::SizeType)0, 
                                         0, 
                                         0, 
                                         _sessionLA->sizePolicy().hasHeightForWidth()) );
  _sessionLA->setMinimumWidth( fontMetrics().width( _sessionLA->text() ) );


  // The "Edit relative" radio button
  lay1->addSpacing(10);
  lay1->addStretch(1);
  _relativeRB = new QRadioButton( i18n( "Edit &relative (apply to both time and"
                                        " session time)" ), page, "_relativeRB" );
  lay1->addWidget( _relativeRB );
  connect( _relativeRB, SIGNAL( clicked() ), this, SLOT(slotRelativePressed()) );
  
  // The relative times
  QHBoxLayout *lay4 = new QHBoxLayout();
  lay1->addLayout( lay4 );
  lay4->addSpacing(20);
  
  _operator = new QComboBox(page);
  _operator->insertItem( QString::fromLatin1( "+" ) );
  _operator->insertItem( QString::fromLatin1( "-" ) );
  _operator->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, 
                                         (QSizePolicy::SizeType)0, 
                                         0, 
                                         0, 
                                         _operator->sizePolicy().hasHeightForWidth()) );
  //kdDebug() << "text width=" << fontMetrics().width( _sessionLA->text() ) << endl;
  _operator->setMinimumWidth( fontMetrics().width( _sessionLA->text() ) );
  lay4->addWidget( _operator );

  _diffTW = new KArmTimeWidget( page, "_sessionAddTW" );
  lay4->addWidget( _diffTW );

  desktopCount = kwinmodule.numberOfDesktops();
  
  // If desktopList contains higher numbered desktops than desktopCount then
  // delete those from desktopList. This may be the case if the user has
  // configured virtual desktops. The values in desktopList are sorted.
  if ( (desktopList != 0) && (desktopList->size() > 0) ) 
  {
    DesktopList::iterator rit = desktopList->begin();
    while (*rit < desktopCount && rit!=desktopList->end()) 
    {
      ++rit;
    }
    desktopList->erase(rit, desktopList->end());
  }

  // The "Choose Desktop" checkbox
  lay1->addSpacing(10);
  lay1->addStretch(1);
  
  _desktopCB = new QCheckBox(i18n("A&uto tracking"), page);
  _desktopCB->setEnabled(true);
  lay1->addWidget(_desktopCB);
  
  Q3GroupBox* groupBox;
  {
    int lines = (int)(desktopCount/2);
    if (lines*2 != desktopCount) lines++; 
      groupBox = new Q3ButtonGroup( lines, Qt::Horizontal,
                                   i18n("In Desktop"), page, "_desktopsGB");
  }
  lay1->addWidget(groupBox);

  QHBoxLayout *lay6 = new QHBoxLayout();

  lay1->addLayout(lay6);
  for (int i=0; i<desktopCount; i++) {
    _deskBox.push_back(new QCheckBox(groupBox,QString::number(i).latin1()));
    _deskBox[i]->setText(kwinmodule.desktopName(i+1));
    _deskBox[i]->setChecked(false);

    lay6->addWidget(_deskBox[i]);
  }
  // check specified Desktop Check Boxes
  bool enableDesktops = false;

  if ( (desktopList != 0) && (desktopList->size() > 0) ) 
  {
    DesktopList::iterator it = desktopList->begin();
    while (it != desktopList->end()) 
    {
      _deskBox[*it]->setChecked(true);
      it++;
    }
    enableDesktops = true;
  }
  // if some desktops were specified, then enable the parent box
  _desktopCB->setChecked(enableDesktops);

  for (int i=0; i<desktopCount; i++)
    _deskBox[i]->setEnabled(enableDesktops);
  
  connect(_desktopCB, SIGNAL(clicked()), this, SLOT(slotAutoTrackingPressed()));

  lay1->addStretch(1);


  if ( editDlg ) {
    // This is an edit dialog.
    _operator->setFocus();
  }
  else {
    // This is an initial dialog
    _name->setFocus();
  }

  slotRelativePressed();

  // Whats this help.
  Q3WhatsThis::add( _name,
                   i18n( "Enter the name of the task here. "
                         "This name is for your eyes only."));
  Q3WhatsThis::add( _absoluteRB,
                   i18n( "Use this option to set the time spent on this task "
                         "to an absolute value.\n\nFor example, if you have "
                         "worked exactly four hours on this task during the current "
                         "session, you would set the Session time to 4 hr." ) );
  Q3WhatsThis::add( _relativeRB,
                   i18n( "Use this option to change the time spent on this task "
                         "relative to its current value.\n\nFor example, if you worked "
                         "on this task for one hour without the timer running, you "
                         "would add 1 hr." ) );
  Q3WhatsThis::add( _timeTW,
                   i18n( "This is the time the task has been "
                         "running since all times were reset."));
  Q3WhatsThis::add( _sessionTW,
                   i18n( "This is the time the task has been running this "
                         "session."));
  Q3WhatsThis::add( _diffTW, i18n( "Specify how much time to add or subtract "
                                  "to the overall and session time"));

  Q3WhatsThis::add( _desktopCB, 
                   i18n( "Use this option to automatically start the timer "
                         "on this task when you switch to the specified desktop(s)." ) );
  Q3WhatsThis::add( groupBox, 
                   i18n( "Select the desktop(s) that will automatically start the "
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

void EditTaskDialog::slotAutoTrackingPressed()
{
  bool checked = _desktopCB->isChecked();
  for (unsigned int i=0; i<_deskBox.size(); i++)
    _deskBox[i]->setEnabled(checked);

  if (!checked)  // uncheck all desktop boxes
    for (int i=0; i<desktopCount; i++) 
      _deskBox[i]->setChecked(false);
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
  if ( _absoluteRB->isChecked() ) {
    *time = _timeTW->time();
    *session = _sessionTW->time();
  }
  else {
    int diff = _diffTW->time();
    if ( _operator->currentItem() == 1) {
      diff = -diff;
    }
    *time = origTime + diff;
    *session = origSession + diff;
  }

  *timeDiff = *time - origTime;
  *sessionDiff = *session - origSession;

  for (unsigned int i=0; i<_deskBox.size(); i++) {
    if (_deskBox[i]->isChecked())
      desktopList->push_back(i);
  }
}

#include "edittaskdialog.moc"
