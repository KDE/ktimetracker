#include "helpers.h"

#include <QTemporaryFile>

#include "taskview.h"

TaskView *createTaskView()
{
    auto *taskView = new TaskView();
    QTemporaryFile icsFile;
    if (!icsFile.open()) {
        delete taskView;
        return nullptr;
    }

    taskView->storage()->load(taskView, QUrl::fromLocalFile(icsFile.fileName()));
    return taskView;
}
