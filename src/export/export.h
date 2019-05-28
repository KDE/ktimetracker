#ifndef KTIMETRACKER_EXPORT_H
#define KTIMETRACKER_EXPORT_H

#include <QString>

#include "reportcriteria.h"

class ProjectModel;
class Task;

QString exportToString(ProjectModel *model, Task *currentTask, const ReportCriteria &rc);
QString writeExport(const ReportCriteria &rc, const QString &data);

#endif //KTIMETRACKER_EXPORT_H
