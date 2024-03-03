/*
    SPDX-FileCopyrightText: 2021 David Stächele <david@daiwai.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KTIMETRACKER_CSVEVENTLOG_H
#define KTIMETRACKER_CSVEVENTLOG_H

#include "model/event.h"
#include "base/reportcriteria.h"

class ProjectModel;

QString exportCSVEventLogToString(ProjectModel *projectModel, const ReportCriteria &rc);
QString getFullEventName(const Event *event, ProjectModel *projectModel);

#endif // KTIMETRACKER_CSVEVENTLOG_H
