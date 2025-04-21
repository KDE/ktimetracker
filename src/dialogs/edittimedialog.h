/*
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef KTIMETRACKER_EDITTIMEDIALOG_H
#define KTIMETRACKER_EDITTIMEDIALOG_H

#include <QDialog>

class KPluralHandlingSpinBox;
class QLabel;
class QDialogButtonBox;

class EditTimeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditTimeDialog(QWidget *parent, const QString &name, const QString &description, int64_t minutes);
    ~EditTimeDialog() override = default;

    int changeMinutes() const
    {
        return m_changeMinutes;
    }
    bool editHistoryRequested() const
    {
        return m_editHistoryRequested;
    }

protected Q_SLOTS:
    void update(int newValue);

private:
    QDialogButtonBox *m_buttonBox;
    KPluralHandlingSpinBox *m_timeEditor;
    QLabel *m_timePreview;
    int m_initialMinutes;
    int m_changeMinutes;
    bool m_editHistoryRequested;
};

#endif // KTIMETRACKER_EDITTIMEDIALOG_H
