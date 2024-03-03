/*
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef KTIMETRACKER_TASKPROPERTIESDIALOG_H
#define KTIMETRACKER_TASKPROPERTIESDIALOG_H

#include <QDialog>
#include <QList>
#include <QScrollArea>


#include "base/desktoplist.h"

class QLineEdit;
class QPlainTextEdit;
class QGroupBox;
class QCheckBox;

class TaskPropertiesDialog : public QDialog
{
public:
    TaskPropertiesDialog(QWidget *parent,
                         const QString &caption,
                         const QString &name,
                         const QString &description,
                         const DesktopList &desktops);
    ~TaskPropertiesDialog() override = default;

    QString name() const;
    QString description() const;
    DesktopList desktops() const;

private:
    QScrollArea *m_scrollArea;
    int m_numDesktops;
    QLineEdit *m_nameText;
    QPlainTextEdit *m_descText;
    QGroupBox *m_trackingGroup;
    QList<QCheckBox *> m_trackingDesktops;
};

#endif // KTIMETRACKER_TASKPROPERTIESDIALOG_H
