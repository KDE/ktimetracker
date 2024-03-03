/*
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef KTIMETRACKER_EXPORT_H
#define KTIMETRACKER_EXPORT_H

#include <QString>

#include "base/reportcriteria.h"

class ProjectModel;
class Task;

QString exportToString(ProjectModel *model, Task *currentTask, const ReportCriteria &rc);
QString writeExport(const QString &data, const QUrl &url);

#endif // KTIMETRACKER_EXPORT_H
