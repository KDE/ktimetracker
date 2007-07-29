/*
 *     Copyright (C) 2000 by Jesper Pedersen <blackie@kde.org>
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

#ifndef KARM_PREFERENCES_H
#define KARM_PREFERENCES_H

#include <KPageDialog>

class QCheckBox;
class QLabel;
class QSpinBox;
class KUrlRequester;

/**
  Provide an interface to the configuration options for the program.
 */
class Preferences : public KPageDialog
{
  Q_OBJECT

  public:
    static Preferences *instance( const QString& icsfile = "" );
    void disableIdleDetection();

    // Retrive information about settings
    bool detectIdleness() const;
    int idlenessTimeout() const;
    int minimumDesktopActiveTime() const;
    QString iCalFile() const;
    QString activeCalendarFile() const;
    bool autoSave() const;
    bool logging() const;
    int autoSavePeriod() const;
    bool promptDelete() const;
    bool uniTasking() const;
    QString setPromptDelete( bool prompt );
    QString setUniTasking( bool b );
    bool displayColumn(int n) const;
    bool decimalFormat() const;
    bool trayIcon() const;
    QString userRealName() const;

    void setDisplayColumn( int column, bool show );

    void emitSignals();
    bool readBoolEntry( const QString& uid );
    void writeEntry( const QString &key, bool value );
    void deleteEntry( const QString &key );

  public Q_SLOTS:
    void showDialog();
    void load();
    void save();

  Q_SIGNALS:
    void detectIdleness(bool on);
    void idlenessTimeout(int minutes);
    void iCalFile(QString);
    void autoSave(bool on);
    void autoSavePeriod(int minutes);
    void setupChanged();

  protected Q_SLOTS:
    virtual void slotButtonClicked(int button);
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
    QCheckBox *_doIdleDetectionW, *_doAutoSaveW, *_promptDeleteW, *_uniTaskingW,
              *_trayIconW;
    QCheckBox *_displayTimeW, *_displaySessionW,
              *_displayTotalTimeW, *_displayTotalSessionW,
	      *_decimalFormatW, *_displayPerCentCompleteW;
    QCheckBox *_loggingW;
    QLabel    *_idleDetectLabelW, *_displayColumnsLabelW, *_minDesktopActiveTimeLabelW;
    QSpinBox  *_idleDetectValueW, *_autoSaveValueW, *_minDesktopActiveTimeValueW;
    KUrlRequester *_iCalFileW ;

    // Values
    bool _doIdleDetectionV, _doAutoSaveV, _promptDeleteV, _loggingV, _uniTaskingV, _trayIconV;
    bool _displayColumnV[5];
    bool _decimalFormatV;
    int  _idleDetectValueV, _autoSaveValueV, _minDesktopActiveTimeValueV;
    QString _iCalFileV;

    /** real name of the user, used during ICAL saving */
    QString _userRealName;
};

#endif // KARM_PREFERENCES_H

