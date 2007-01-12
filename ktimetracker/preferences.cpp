#undef Unsorted // for --enable-final
#include <QCheckBox>
#include <QLabel>
#include <QString>
#include <QSpinBox>
#include <QLayout>
#include <QPixmap>
#include <QVBoxLayout>
#include <QGridLayout>

#include <kapplication.h>       // kapp
#include <kconfig.h>
#include <kdebug.h>
#include <kemailsettings.h>
#include <kiconloader.h>
#include <klineedit.h>          // lineEdit()
#include <klocale.h>            // i18n
#include <kstandarddirs.h>
#include <kurlrequester.h>
#include <kglobal.h>
#include <kicon.h>

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
  KIcon icon = KIcon( SmallIconSet( "kcmsystem", K3Icon::SizeMedium) );
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
  KIcon icon = KIcon( SmallIconSet( "viewmag", K3Icon::SizeMedium ) );

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
  KIcon icon = KIcon( SmallIconSet( "kfm", K3Icon::SizeMedium ) );
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

//---------------------------------------------------------------------------
//                                  Load and Save
//---------------------------------------------------------------------------
void Preferences::load()
{
  KConfig &config = *KGlobal::config();

  config.setGroup( QString::fromLatin1("Idle detection") );
  _doIdleDetectionV = config.readEntry( QString::fromLatin1("enabled"),
     true );
  _idleDetectValueV = config.readEntry(QString::fromLatin1("period"), 15);

  config.setGroup( QString::fromLatin1("Saving") );
  _iCalFileV = config.readPathEntry
    ( QString::fromLatin1("ical file"),
      KStandardDirs::locateLocal( "appdata", QString::fromLatin1( "karm.ics")));
  _doAutoSaveV = config.readEntry
    ( QString::fromLatin1("auto save"), true);
  _autoSaveValueV = config.readEntry
    ( QString::fromLatin1("auto save period"), 5);
  _promptDeleteV = config.readEntry
    ( QString::fromLatin1("prompt delete"), true);
  _uniTaskingV = config.readEntry
    ( QString::fromLatin1("unitasking"), false);
  _loggingV = config.readEntry
    ( QString::fromLatin1("logging"), true);

  _displayColumnV[0] = config.readEntry
    ( QString("display session time"), true);
  _displayColumnV[1] = config.readEntry
    ( QString("display time"), true);
  _displayColumnV[2] = config.readEntry
    ( QString("display total session time"), true);
  _displayColumnV[3] = config.readEntry
    ( QString("display total time"), true);
  _displayColumnV[4] = config.readEntry
    ( QString("display percent complete"), false);
  
  _trayIconV = config.readEntry
    ( QString::fromLatin1("tray icon"), true);

  KEMailSettings settings;
  _userRealName = settings.getSetting( KEMailSettings::RealName );
}

void Preferences::save()
{
  kDebug(5970) << "Entering Preferences::save" << endl;
  KConfig &config = *KGlobal::config();

  config.setGroup( QString("Idle detection"));
  config.writeEntry( QString("enabled"), _doIdleDetectionV);
  config.writeEntry( QString("period"), _idleDetectValueV);

  config.setGroup( QString("Saving"));
  config.writePathEntry( QString("ical file"), _iCalFileV);
  config.writeEntry( QString("auto save"), _doAutoSaveV);
  config.writeEntry( QString("logging"), _loggingV);
  config.writeEntry( QString("auto save period"), _autoSaveValueV);
  config.writeEntry( QString("prompt delete"), _promptDeleteV);
  config.writeEntry( QString("unitasking"), _uniTaskingV);

  config.writeEntry( QString("display session time"), _displayColumnV[0] );
  config.writeEntry( QString("display time"), _displayColumnV[1] );
  config.writeEntry( QString("display total session time"), _displayColumnV[2] );
  config.writeEntry( QString("display total time"), _displayColumnV[3] );
  config.writeEntry( QString("display percent complete"), _displayColumnV[4] );
  config.writeEntry( QString("tray icon"), _trayIconV );
  config.sync();
}

// HACK: this entire config dialog should be upgraded to KConfigXT
bool Preferences::readBoolEntry( const QString& key )
{
  KConfig &config = *KGlobal::config();
  return config.readEntry ( key, true );
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
