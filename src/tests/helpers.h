/*
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef KTIMETRACKER_HELPERS_H
#define KTIMETRACKER_HELPERS_H

#include <QObject>
#include <QString>

class TaskView;

QUrl createTempFile(QObject *parent);
TaskView *createTaskView(QObject *parent, bool simpleTree);
QString readTextFile(const QString &path);

#endif // KTIMETRACKER_HELPERS_H
