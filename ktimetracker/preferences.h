#include <kdialogbase.h>

class QCheckBox;
class QLabel;
class QSpinBox;
class QLineEdit;


class Preferences :public KDialogBase 
{
Q_OBJECT

public:
  static Preferences *instance();
  
   // Retrive information about settings
  bool detectIdleness();
  int idlenessTimeout();
  QString saveFile();
  bool autoSave();
  int autoSavePeriod();

public slots:
  void showDialog();
  void load();
  void save();
  

signals:  
  void detectIdleness(bool on);
  void idlenessTimeout(int minutes);
  void saveFile(QString);
  void autoSave(bool on);
  void autoSavePeriod(int minutes);
  void setupChanged();
  
protected slots:
  virtual void slotOk();
  virtual void slotCancel();
  void idleDetectCheckBoxChanged();
  void autoSaveCheckBoxChanged();
  
protected:
  void emitSignals();

private:
  Preferences();
  static Preferences *_instance;
  bool _unsavedChanges;

  // Widgets in the dialog (All variables ends in W to indicate that they are Widgets)
  QCheckBox *_doIdleDetectionW, *_doAutoSaveW;
  QLabel    *_idleDetectLabelW, *_autoSaveLabelW;
  QSpinBox  *_idleDetectValueW, *_autoSaveValueW;
  QLineEdit *_saveFileW;

  // Values for the preferences. (All variables in in V to indicate they are Values)
  bool _doIdleDetectionV, _doAutoSaveV;
  int  _idleDetectValueV, _autoSaveValueV;
  QString _saveFileV;
  
};


  
