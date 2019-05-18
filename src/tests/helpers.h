#ifndef KTIMETRACKER_HELPERS_H
#define KTIMETRACKER_HELPERS_H

#include <QObject>
#include <QString>

class TaskView;

TaskView *createTaskView(QObject *parent, bool simpleTree);
QString readTextFile(const QString &path);

#endif // KTIMETRACKER_HELPERS_H
