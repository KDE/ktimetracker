#undef Unsorted // for --enable-final
#include <qcheckbox.h>
#include <qlabel.h>
#include <qstring.h>
#include <qspinbox.h>
#include <qlayout.h>

#include <kapplication.h>       // kapp
#include <kconfig.h>
#include <kdebug.h>
#include <kemailsettings.h>
#include <kiconloader.h>
#include <klineedit.h>          // lineEdit()
#include <klocale.h>            // i18n
#include <kstandarddirs.h>
#include <kurlrequester.h>

#include "preferences.h"

Preferences *Preferences::_instance = 0;

Preferences::Preferences( const QString& icsFile )
  : KDialogBase( IconList, i18n("Preferences"), Ok|Cancel, Ok )
{

  setIconListAllVisible( true );

  makeBehaviorPage();
  makeDisplayPage();
  makeStoragePage();

  load();

  // command-line option overrides what is stored in 
  if ( ! icsFile.isEmpty() ) _iCalFileV = icsFile;

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
  QPixmap icon = SmallIcon( "kcmsystem", KIcon::SizeMedium);
  QFrame* behaviorPage = addPage( i18n("Behavior"), i18n("Behavior Settings"),
      icon );

  QVBoxLayout* topLevel = new QVBoxLayout( behaviorPage, 0, spacingHint() );
  QGridLayout* layout = new QGridLayout( topLevel, 2, 2 );
  layout->setColStretch( 1, 1 );

  _doIdleDetectionW = new QCheckBox
    ( i18n("Detect desktop as idle after"), behaviorPage, "_doIdleDetectionW");
  _idleDetectValueW = new QSpinBox
    (1,60*24, 1, behaviorPage, "_idleDetectValueW");
  _idleDetectValueW->setSuffix(i18n(" min"));
  _promptDeleteW = new QCheckBox
    ( i18n( "Prompt before deleting tasks" ), behaviorPage, "_promptDeleteW" );

  layout->addWidget(_doIdleDetectionW, 0, 0 );
  layout->addWidget(_idleDetectValueW, 0, 1 );
  layout->addWidget(_promptDeleteW, 1, 0 );

  topLevel->addStretch();

  connect( _doIdleDetectionW, SIGNAL( clicked() ), this,
      SLOT( idleDetectCheckBoxChanged() ));
}

void Preferences::makeDisplayPage()
{
  QPixmap icon = SmallIcon( "viewmag", KIcon::SizeMedium );
  QFrame* displayPage = addPage( i18n("Display"), i18n("Display Settings"),
      icon );

  QVBoxLayout* topLevel = new QVBoxLayout( displayPage, 0, spacingHint() );
  QGridLayout* layout = new QGridLayout( topLevel, 5, 2 );
  layout->setColStretch( 1, 1 );

  QLabel* _displayColumnsLabelW = new QLabel( i18n("Columns displayed:"),
      displayPage );
  _displaySessionW = new QCheckBox ( i18n("Session time"),
      displayPage, "_displaySessionW");
  _displayTimeW = new QCheckBox ( i18n("Cumulative task time"),
      displayPage, "_displayTimeW");
  _displayTotalSessionW = new QCheckBox( i18n("Total session time"),
      displayPage, "_displayTotalSessionW");
  _displayTotalTimeW = new QCheckBox ( i18n("Total task time"),
      displayPage, "_displayTotalTimeW");

  layout->addMultiCellWidget( _displayColumnsLabelW, 0, 0, 0, 1 );
  layout->addWidget(_displaySessionW, 1, 1 );
  layout->addWidget(_displayTimeW, 2, 1 );
  layout->addWidget(_displayTotalSessionW, 3, 1 );
  layout->addWidget(_displayTotalTimeW, 4, 1 );

  topLevel->addStretch();
}

void Preferences::makeStoragePage()
{
  QPixmap icon = SmallIcon( "kfm", KIcon::SizeMedium );
  QFrame* storagePage = addPage( i18n("Storage"), i18n("Storage Settings"),
      icon );

  QVBoxLayout* topLevel = new QVBoxLayout( storagePage, 0, spacingHint() );
  QGridLayout* layout = new QGridLayout( topLevel, 4, 2 );
  layout->setColStretch( 1, 1 );

  // autosave
  _doAutoSaveW = new QCheckBox
    ( i18n("Save tasks every"), storagePage, "_doAutoSaveW" );
  _autoSaveValueW = new QSpinBox(1, 60*24, 1, storagePage, "_autoSaveValueW");
  _autoSaveValueW->setSuffix(i18n(" min"));

  // iCalendar
  QLabel* _iCalFileLabel = new QLabel( i18n("iCalendar file:"), storagePage);
  _iCalFileW = new KURLRequester(storagePage, "_iCalFileW");
  _iCalFileW->setFilter(QString::fromLatin1("*.ics"));
  _iCalFileW->setMode(KFile::File);

  // Log time?
  _loggingW = new QCheckBox 
    ( i18n("Log history"), storagePage, "_loggingW" );

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

  _displaySessionW->setChecked(_displayColumnV[0]);
  _displayTimeW->setChecked(_displayColumnV[1]);
  _displayTotalSessionW->setChecked(_displayColumnV[2]);
  _displayTotalTimeW->setChecked(_displayColumnV[3]);

  // adapt visibility of preference items according
  // to settings
  idleDetectCheckBoxChanged();
  autoSaveCheckBoxChanged();

  show();
}

void Preferences::slotOk()
{

  // storage
  _iCalFileV = _iCalFileW->lineEdit()->text();

  _doIdleDetectionV = _doIdleDetectionW->isChecked();
  _idleDetectValueV = _idleDetectValueW->value();

  _doAutoSaveV = _doAutoSaveW->isChecked();
  _autoSaveValueV = _autoSaveValueW->value();
  _loggingV = _loggingW->isChecked();

  // behavior
  _promptDeleteV = _promptDeleteW->isChecked();

  // display
  _displayColumnV[0] = _displaySessionW->isChecked();
  _displayColumnV[1] = _displayTimeW->isChecked();
  _displayColumnV[2] = _displayTotalSessionW->isChecked();
  _displayColumnV[3] = _displayTotalTimeW->isChecked();

  emitSignals();
  save();
  KDialogBase::slotOk();
}

void Preferences::slotCancel()
{
  KDialogBase::slotCancel();
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
QString Preferences::setPromptDelete(bool prompt)    { _promptDeleteV=prompt; return ""; }
bool    Preferences::displayColumn(int n)            const { return _displayColumnV[n]; }
QString Preferences::userRealName()                  const { return _userRealName; }

//---------------------------------------------------------------------------
//                                  Load and Save
//---------------------------------------------------------------------------
void Preferences::load()
{
  KConfig &config = *kapp->config();

  config.setGroup( QString::fromLatin1("Idle detection") );
  _doIdleDetectionV = config.readBoolEntry( QString::fromLatin1("enabled"),
     true );
  _idleDetectValueV = config.readNumEntry(QString::fromLatin1("period"), 15);

  config.setGroup( QString::fromLatin1("Saving") );
  _iCalFileV = config.readPathEntry
    ( QString::fromLatin1("ical file"), 
      locateLocal( "appdata", QString::fromLatin1( "karm.ics")));
  _doAutoSaveV = config.readBoolEntry
    ( QString::fromLatin1("auto save"), true);
  _autoSaveValueV = config.readNumEntry
    ( QString::fromLatin1("auto save period"), 5);
  _promptDeleteV = config.readBoolEntry
    ( QString::fromLatin1("prompt delete"), true);
  _loggingV = config.readBoolEntry
    ( QString::fromLatin1("logging"), true);

  _displayColumnV[0] = config.readBoolEntry
    ( QString::fromLatin1("display session time"), true);
  _displayColumnV[1] = config.readBoolEntry
    ( QString::fromLatin1("display time"), true);
  _displayColumnV[2] = config.readBoolEntry
    ( QString::fromLatin1("display total session time"), true);
  _displayColumnV[3] = config.readBoolEntry
    ( QString::fromLatin1("display total time"), true);

  KEMailSettings settings;
  _userRealName = settings.getSetting( KEMailSettings::RealName );
}

void Preferences::save()
{
  KConfig &config = *KGlobal::config();

  config.setGroup( QString::fromLatin1("Idle detection"));
  config.writeEntry( QString::fromLatin1("enabled"), _doIdleDetectionV);
  config.writeEntry( QString::fromLatin1("period"), _idleDetectValueV);

  config.setGroup( QString::fromLatin1("Saving"));
  config.writePathEntry( QString::fromLatin1("ical file"), _iCalFileV);
  config.writeEntry( QString::fromLatin1("auto save"), _doAutoSaveV);
  config.writeEntry( QString::fromLatin1("logging"), _loggingV);
  config.writeEntry( QString::fromLatin1("auto save period"), _autoSaveValueV);
  config.writeEntry( QString::fromLatin1("prompt delete"), _promptDeleteV);

  config.writeEntry( QString::fromLatin1("display session time"),
      _displayColumnV[0]);
  config.writeEntry( QString::fromLatin1("display time"),
      _displayColumnV[1]);
  config.writeEntry( QString::fromLatin1("display total session time"),
      _displayColumnV[2]);
  config.writeEntry( QString::fromLatin1("display total time"),
      _displayColumnV[3]);

  config.sync();
}

// HACK: this entire config dialog should be upgraded to KConfigXT
bool Preferences::readBoolEntry( const QString& key )
{
  KConfig &config = *KGlobal::config();
  return config.readBoolEntry ( key, true );
}

void Preferences::writeEntry( const QString &key, bool value)
{
  KConfig &config = *KGlobal::config();
  config.writeEntry( key, value );
  config.sync();
}

void Preferences::deleteEntry( const QString &key )
{
  KConfig &config = *KGlobal::config();
  config.deleteEntry( key );
  config.sync();
}

#include "preferences.moc"
