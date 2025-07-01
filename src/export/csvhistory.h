/*
    SPDX-FileCopyrightText: 2003, 2004 Mark Bucciarelli <mark@hubcapconsutling.com>
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KTIMETRACKER_CSVHISTORY_H
#define KTIMETRACKER_CSVHISTORY_H

#include "base/reportcriteria.h"

class Task;
class ProjectModel;

QString exportCSVHistoryToString(ProjectModel *projectModel, Task *currentItem, const ReportCriteria &rc);

#endif // KTIMETRACKER_CSVHISTORY_H
