#include "helpers.h"

#include <QTemporaryFile>

#include "taskview.h"
#include "model/task.h"

TaskView *createTaskView(bool simpleTree)
{
    auto *taskView = new TaskView();
    QTemporaryFile icsFile;
    if (!icsFile.open()) {
        delete taskView;
        return nullptr;
    }

    taskView->storage()->load(taskView, QUrl::fromLocalFile(icsFile.fileName()));

    if (simpleTree) {
        Task* task1 = taskView->task(taskView->addTask("1"));
        Task* task2 = taskView->task(taskView->addTask("2", QString(), 0, 0, QVector<int>(0, 0), task1));
        Task* task3 = taskView->task(taskView->addTask("3"));

        task1->changeTime(5, nullptr); // add 5 minutes
        task2->changeTime(3, nullptr); // add 3 minutes
        task3->changeTime(7, nullptr); // add 7 minutes
    }

    return taskView;
}
