/*
 *     Copyright (C) 2007 the ktimetracker developers
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

#undef Unsorted // for --enable-final
#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>
#include <QLayout>
#include <QPixmap>
#include <QSpinBox>
#include <QString>
#include <QVBoxLayout>

#include <KApplication>       // kapp
#include <KConfig>
#include <KDebug>
#include <KGlobal>
#include <KIcon>
#include <KIconLoader>
#include <KLineEdit>          // lineEdit()
#include <KLocale>            // i18n
#include <KStandardDirs>
#include <KUrlRequester>

#include <kemailsettings.h>

#include "preferences.h"

Preferences *Preferences::_instance = 0;

Preferences::Preferences( const QString& icsFile )
  : KPageDialog()
{
  setCaption( i18n("Preferences") );
  setButtons( Ok|Cancel );
  setDefaultButton(  Ok );
  setFaceType( KPageDialog::List );

  //setIconListAllVisible( true );

  makeBehaviorPage();
  makeDisplayPage();
  makeStoragePage();

  load();

  // command-line option overrides what is stored in
  if ( ! icsFile.isEmpty() ) _iCalFileV = icsFile;

  connect(this,SIGNAL(okClicked()),SLOT(slotOk()));
  connect(this,SIGNAL(cancelClicked()),SLOT(slotCancel()));
}

Preferences *Preferences::instance( const QString &icsfile )
{
  if (_instance == 0) {
    _instance = new Preferences( icsfile );
  }
  return _instance;
}

void Preferences::makeBehaviorPage()
{
  KIcon icon = KIcon( "kcmsystem" );
  QFrame* behaviorPage = new QFrame();
  KPageWidgetItem *pageItem = new KPageWidgetItem( behaviorPage, i18n("Behavior"));
  pageItem->setHeader( i18n("Behavior Settings") );
  pageItem->setIcon( icon );
  addPage(pageItem);

  QVBoxLayout* topLevel = new QVBoxLayout( behaviorPage );
  topLevel->setSpacing( spacingHint() );
  topLevel->setMargin( 0 );
  QGridLayout* layout = new QGridLayout();
  topLevel->addLayout( layout );
  layout->setColumnStretch( 1, 1 );

  _doIdleDetectionW = new QCheckBox
    ( i18n("Detect desktop as idle after"), behaviorPage);
  _doIdleDetectionW->setObjectName( "_doIdleDetectionW" );
  _idleDetectValueW = new QSpinBox(behaviorPage);
  _idleDetectValueW->setObjectName( "_idleDetectValueW" );
  _idleDetectValueW->setRange(1,30*24);
  _idleDetectValueW->setSuffix(i18n(" min"));
  _promptDeleteW = new QCheckBox
    ( i18n( "Prompt before deleting tasks" ), behaviorPage );
  _promptDeleteW->setObjectName( "_promptDeleteW" );
  _uniTaskingW = new QCheckBox
    ( i18n( "Allow only one timer at a time" ), behaviorPage );
  _uniTaskingW->setObjectName( "_uniTaskingW" );
  _uniTaskingW->setWhatsThis( i18n("Unitasking - allow only one task to be timed at a time. Does not stop any timer.") );
  _trayIconW = new QCheckBox
    ( i18n( "Place an icon to the SysTray" ), behaviorPage );
  _trayIconW->setObjectName( "_trayIcon" );

  layout->addWidget(_doIdleDetectionW, 0, 0 );
  layout->addWidget(_idleDetectValueW, 0, 1 );
  layout->addWidget(_promptDeleteW, 1, 0 );
  layout->addWidget(_uniTaskingW, 2, 0);
  layout->addWidget(_trayIconW, 3, 0);

  topLevel->addStretch();

  connect( _doIdleDetectionW, SIGNAL( clicked() ), this,
      SLOT( idleDetectCheckBoxChanged() ));
}

void Preferences::makeDisplayPage()
{
  KIcon icon = KIcon( "zoom-original" );

  QFrame* displayPage = new QFrame();
  KPageWidgetItem *pageItem = new KPageWidgetItem( displayPage, i18n("Display"));
  pageItem->setHeader( i18n("Display Settings") );
  pageItem->setIcon( icon );
  addPage(pageItem);

  QVBoxLayout* topLevel = new QVBoxLayout( displayPage );
  topLevel->setSpacing( spacingHint() );
  topLevel->setMargin( 0 );
  QGridLayout* layout = new QGridLayout();
  topLevel->addLayout( layout );
  layout->setColumnStretch( 1, 1 );

  QLabel* _displayColumnsLabelW = new QLabel( i18n("Columns displayed:"),
      displayPage );
  _displaySessionW = new QCheckBox ( i18n("Session time"),
      displayPage);
  _displaySessionW->setObjectName( "_displaySessionW" );
  _displayTimeW = new QCheckBox ( i18n("Cumulative task time"),
      displayPage);
  _displayTimeW->setObjectName( "_displayTimeW" );
  _displayTotalSessionW = new QCheckBox( i18n("Total session time"),
      displayPage);
  _displayTotalSessionW->setObjectName( "_displayTotalSessionW" );
  _displayTotalTimeW = new QCheckBox ( i18n("Total task time"),
      displayPage);
  _displayTotalTimeW->setObjectName( "_displayTotalTimeW" );
  _displayPerCentCompleteW = new QCheckBox ( i18n("Percent Complete"),
      displayPage);
  _displayPerCentCompleteW->setObjectName( "_perCentCompleteW" );

  QLabel* _numberFormatW = new QLabel( i18n("Number format:"),
      displayPage );
  _decimalFormatW = new QCheckBox( i18n("Decimal"), displayPage );
  _decimalFormatW->setObjectName( "_decimalDisplayW" );

  layout->addWidget( _displayColumnsLabelW, 0, 1, 1, 2 );
  layout->addWidget(_displaySessionW, 1, 1 );
  layout->addWidget(_displayTimeW, 2, 1 );
  layout->addWidget(_displayTotalSessionW, 3, 1 );
  layout->addWidget(_displayTotalTimeW, 4, 1 );
  layout->addWidget(_displayPerCentCompleteW, 5, 1 );

  layout->addWidget( _numberFormatW, 0, 2, 1, 1);
  layout->addWidget( _decimalFormatW, 1, 2 );

  topLevel->addStretch();
}

void Preferences::makeStoragePage()
{
  KIcon icon = KIcon( "kfm" );
  QFrame* storagePage = new QFrame();
  KPageWidgetItem *pageItem = new KPageWidgetItem( storagePage, i18n("Storage") );
  pageItem->setHeader( i18n("Storage Settings") );
  pageItem->setIcon( icon );
  addPage(pageItem);

  QVBoxLayout* topLevel = new QVBoxLayout( storagePage );
  topLevel->setSpacing( spacingHint() );
  topLevel->setMargin( 0 );
  QGridLayout* layout = new QGridLayout();
  topLevel->addLayout( layout );
  layout->setColumnStretch( 1, 1 );

  // autosave
  _doAutoSaveW = new QCheckBox( i18n("Save tasks every"), storagePage );
  _doAutoSaveW->setObjectName( "_doAutoSaveW" );
  _autoSaveValueW = new QSpinBox(storagePage);
  _autoSaveValueW->setObjectName( "_autoSaveValueW" );
  _autoSaveValueW->setRange(1, 60*24);
  _autoSaveValueW->setSuffix(i18n(" min"));

  // iCalendar
  QLabel* _iCalFileLabel = new QLabel( i18n("iCalendar file:"), storagePage );
  _iCalFileW = new KUrlRequester( storagePage );
  _iCalFileW->setFilter(QString::fromLatin1("*.ics"));
  _iCalFileW->setMode(KFile::File);

  // Log time?
  _loggingW = new QCheckBox( i18n("Log history"), storagePage );
  _loggingW->setObjectName( "_loggingW" );

  // add widgets to layout
  layout->addWidget(_doAutoSaveW, 0, 0);
  layout->addWidget(_autoSaveValueW, 0, 1);
  layout->addWidget(_iCalFileLabel, 1, 0 );
  layout->addWidget(_iCalFileW, 1, 1 );
  layout->addWidget(_loggingW, 2, 0 );

  topLevel->addStretch();

  // checkboxes disable file selection controls
  connect( _doAutoSaveW, SIGNAL( clicked() ),
      this, SLOT( autoSaveCheckBoxChanged() ));
}

void Preferences::disableIdleDetection()
{
  _doIdleDetectionW->setEnabled(false);
}


//---------------------------------------------------------------------------
//                            SLOTS
//---------------------------------------------------------------------------

void Preferences::showDialog()
{

  // set all widgets
  _iCalFileW->lineEdit()->setText(_iCalFileV);

  _doIdleDetectionW->setChecked(_doIdleDetectionV);
  _idleDetectValueW->setValue(_idleDetectValueV);

  _doAutoSaveW->setChecked(_doAutoSaveV);
  _autoSaveValueW->setValue(_autoSaveValueV);
  _loggingW->setChecked(_loggingV);

  _promptDeleteW->setChecked(_promptDeleteV);
  _uniTaskingW->setChecked(_uniTaskingV);

  _displaySessionW->setChecked(_displayColumnV[0]);
  _displayTimeW->setChecked(_displayColumnV[1]);
  _displayTotalSessionW->setChecked(_displayColumnV[2]);
  _displayTotalTimeW->setChecked(_displayColumnV[3]);
  _displayPerCentCompleteW->setChecked(_displayColumnV[4]);

  _trayIconW->setChecked(_trayIconV);

  // adapt visibility of preference items according
  // to settings
  idleDetectCheckBoxChanged();
  autoSaveCheckBoxChanged();

  show();
}

void Preferences::slotButtonClicked(int button)
{
  kDebug(5970) << "Entering Preferences::slotButtonClicked" << endl;
  if (button == KDialog::Ok) slotOk();
  if (button == KDialog::Cancel) slotCancel();
}

void Preferences::slotOk()
{
  kDebug(5970) << "Entering Preferences::slotOk" << endl;

  // storage
  _iCalFileV = _iCalFileW->lineEdit()->text();

  _doIdleDetectionV = _doIdleDetectionW->isChecked();
  _idleDetectValueV = _idleDetectValueW->value();

  _doAutoSaveV = _doAutoSaveW->isChecked();
  _autoSaveValueV = _autoSaveValueW->value();
  _loggingV = _loggingW->isChecked();

  // behavior
  _promptDeleteV = _promptDeleteW->isChecked();
  _uniTaskingV = _uniTaskingW->isChecked();

  // display
  _displayColumnV[0] = _displaySessionW->isChecked();
  _displayColumnV[1] = _displayTimeW->isChecked();
  _displayColumnV[2] = _displayTotalSessionW->isChecked();
  _displayColumnV[3] = _displayTotalTimeW->isChecked();
  _displayColumnV[4] = _displayPerCentCompleteW->isChecked();
  _decimalFormatV = _decimalFormatW->isChecked();
  _trayIconV = _trayIconW->isChecked();

  emitSignals();
  save();
  KDialog::accept();
}

void Preferences::slotCancel()
{
  kDebug(5970) << "Entering Preferences::slotCancel" << endl;
  KDialog::reject();
}

void Preferences::idleDetectCheckBoxChanged()
{
  _idleDetectValueW->setEnabled(_doIdleDetectionW->isChecked());
}

void Preferences::autoSaveCheckBoxChanged()
{
  _autoSaveValueW->setEnabled(_doAutoSaveW->isChecked());
}

void Preferences::emitSignals()
{
  kDebug(5970) << "Entering Preferences::emitSignals" << endl;
  emit iCalFile( _iCalFileV );
  emit detectIdleness( _doIdleDetectionV );
  emit idlenessTimeout( _idleDetectValueV );
  emit autoSave( _doAutoSaveV );
  emit autoSavePeriod( _autoSaveValueV );
  emit setupChanged();
}

QString Preferences::iCalFile()           	     const { return _iCalFileV; }
QString Preferences::activeCalendarFile() 	     const { return _iCalFileV; }
bool    Preferences::detectIdleness()                const { return _doIdleDetectionV; }
int     Preferences::idlenessTimeout()               const { return _idleDetectValueV; }
bool    Preferences::autoSave()                      const { return _doAutoSaveV; }
int     Preferences::autoSavePeriod()                const { return _autoSaveValueV; }
bool    Preferences::logging()                       const { return _loggingV; }
bool    Preferences::promptDelete()                  const { return _promptDeleteV; }
bool    Preferences::uniTasking()                    const { return _uniTaskingV; }
QString Preferences::setPromptDelete(bool prompt)    { _promptDeleteV=prompt; return ""; }
QString Preferences::setUniTasking(bool b)           { _uniTaskingV=b; return ""; }
bool    Preferences::displayColumn(int n)            const { return _displayColumnV[n]; }
QString Preferences::userRealName()                  const { return _userRealName; }
bool    Preferences::decimalFormat()		     const { return _decimalFormatV; }
bool    Preferences::trayIcon()                      const { return _trayIconV; }

void Preferences::setDisplayColumn( int column, bool show )
{
  _displayColumnV[column] = show;
  emitSignals();
}

//---------------------------------------------------------------------------
//                                  Load and Save
//---------------------------------------------------------------------------
void Preferences::load()
{
  KConfigGroup configIdleDetection = KGlobal::config()->group( QString::fromLatin1("Idle detection") );
  _doIdleDetectionV = configIdleDetection.readEntry( QString::fromLatin1("enabled"),
     true );
  _idleDetectValueV = configIdleDetection.readEntry(QString::fromLatin1("period"), 15);

  KConfigGroup configSaving = KGlobal::config()->group( QString::fromLatin1("Saving") );
  _iCalFileV = configSaving.readPathEntry
    ( QString::fromLatin1("ical file"),
      KStandardDirs::locateLocal( "appdata", QString::fromLatin1( "karm.ics")));
  _doAutoSaveV = configSaving.readEntry
    ( QString::fromLatin1("auto save"), true);
  _autoSaveValueV = configSaving.readEntry
    ( QString::fromLatin1("auto save period"), 5);
  _promptDeleteV = configSaving.readEntry
    ( QString::fromLatin1("prompt delete"), true);
  _uniTaskingV = configSaving.readEntry
    ( QString::fromLatin1("unitasking"), false);
  _loggingV = configSaving.readEntry
    ( QString::fromLatin1("logging"), true);

  _displayColumnV[0] = configSaving.readEntry
    ( QString("display session time"), true);
  _displayColumnV[1] = configSaving.readEntry
    ( QString("display time"), true);
  _displayColumnV[2] = configSaving.readEntry
    ( QString("display total session time"), true);
  _displayColumnV[3] = configSaving.readEntry
    ( QString("display total time"), true);
  _displayColumnV[4] = configSaving.readEntry
    ( QString("display percent complete"), false);
  
  _trayIconV = configSaving.readEntry
    ( QString::fromLatin1("tray icon"), true);

  KEMailSettings settings;
  _userRealName = settings.getSetting( KEMailSettings::RealName );
}

void Preferences::save()
{
  kDebug(5970) << "Entering Preferences::save" << endl;
  KConfigGroup configIdleDetection = KGlobal::config()->group( QString("Idle detection") );
  configIdleDetection.writeEntry( QString("enabled"), _doIdleDetectionV);
  configIdleDetection.writeEntry( QString("period"), _idleDetectValueV);
  configIdleDetection.sync();

  KConfigGroup configSaving = KGlobal::config()->group( QString("Saving") );
  configSaving.writePathEntry( QString("ical file"), _iCalFileV);
  configSaving.writeEntry( QString("auto save"), _doAutoSaveV);
  configSaving.writeEntry( QString("logging"), _loggingV);
  configSaving.writeEntry( QString("auto save period"), _autoSaveValueV);
  configSaving.writeEntry( QString("prompt delete"), _promptDeleteV);
  configSaving.writeEntry( QString("unitasking"), _uniTaskingV);

  configSaving.writeEntry( QString("display session time"), _displayColumnV[0] );
  configSaving.writeEntry( QString("display time"), _displayColumnV[1] );
  configSaving.writeEntry( QString("display total session time"), _displayColumnV[2] );
  configSaving.writeEntry( QString("display total time"), _displayColumnV[3] );
  configSaving.writeEntry( QString("display percent complete"), _displayColumnV[4] );
  configSaving.writeEntry( QString("tray icon"), _trayIconV );
  configSaving.sync();
}

// HACK: this entire config dialog should be upgraded to KConfigXT
bool Preferences::readBoolEntry( const QString& key )
{
  return KGlobal::config()->group( QString() ).readEntry( key, true );
}

void Preferences::writeEntry( const QString &key, bool value)
{
  KConfigGroup config = KGlobal::config()->group( QString() );
  config.writeEntry( key, value );
  config.sync();
}

void Preferences::deleteEntry( const QString &key )
{
  KConfigGroup config = KGlobal::config()->group( QString() );
  config.deleteEntry( key );
  config.sync();
}

#include "preferences.moc"
