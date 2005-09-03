#ifndef KARM_PREFERENCES_H
#define KARM_PREFERENCES_H

#include <kdialogbase.h>
//Added by qt3to4:
#include <QLabel>

class QCheckBox;
class QLabel;
class QSpinBox;
class QString;
class KURLRequester;

/**
 * Provide an interface to the configuration options for the program.
 */

class Preferences :public KDialogBase
{
  Q_OBJECT

  public:
    static Preferences *instance( const QString& icsfile = "" );
    void disableIdleDetection();

    // Retrive information about settings
    bool detectIdleness() const;
    int idlenessTimeout() const;
    QString iCalFile() const;
    QString activeCalendarFile() const;
    bool autoSave() const;
    bool logging() const;
    int autoSavePeriod() const;
    bool promptDelete() const;
    QString setPromptDelete( bool prompt );
    bool displayColumn(int n) const;
    QString userRealName() const;

    void emitSignals();
    bool readBoolEntry( const QString& uid );
    void writeEntry( const QString &key, bool value );
    void deleteEntry( const QString &key );

  public slots:
    void showDialog();
    void load();
    void save();

  signals:
    void detectIdleness(bool on);
    void idlenessTimeout(int minutes);
    void iCalFile(QString);
    void autoSave(bool on);
    void autoSavePeriod(int minutes);
    void setupChanged();

  protected slots:
    virtual void slotOk();
    virtual void slotCancel();
    void idleDetectCheckBoxChanged();
    void autoSaveCheckBoxChanged();

  private:
    void makeDisplayPage();
    void makeBehaviorPage();
    void makeStoragePage();

    Preferences( const QString& icsfile = "" );
    static Preferences *_instance;
    bool _unsavedChanges;

    // Widgets
    QCheckBox *_doIdleDetectionW, *_doAutoSaveW, *_promptDeleteW;
    QCheckBox *_displayTimeW, *_displaySessionW,
              *_displayTotalTimeW, *_displayTotalSessionW;
    QCheckBox *_loggingW;
    QLabel    *_idleDetectLabelW, *_displayColumnsLabelW;
    QSpinBox  *_idleDetectValueW, *_autoSaveValueW;
    KURLRequester *_iCalFileW ;

    // Values
    bool _doIdleDetectionV, _doAutoSaveV, _promptDeleteV, _loggingV;
    bool _displayColumnV[4];
    int  _idleDetectValueV, _autoSaveValueV;
    QString _iCalFileV;

    /** real name of the user, used during ICAL saving */
    QString _userRealName;
};

#endif // KARM_PREFERENCES_H

