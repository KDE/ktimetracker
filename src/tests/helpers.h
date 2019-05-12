#ifndef KTIMETRACKER_HELPERS_H
#define KTIMETRACKER_HELPERS_H

#include <QString>

class TaskView;

TaskView *createTaskView(bool simpleTree);
QString readTextFile(const QString &path);

#endif // KTIMETRACKER_HELPERS_H
