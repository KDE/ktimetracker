#include "helpers.h"

#include <QTemporaryFile>
#include <QTextStream>

#include "taskview.h"
#include "model/task.h"

TaskView *createTaskView(bool simpleTree)
{
    auto *taskView = new TaskView();
    // TODO: remove all temporary files after running test
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

        task1->changeTime(5, taskView->storage()); // add 5 minutes
        task2->changeTime(3, taskView->storage()); // add 3 minutes
        task3->changeTime(7, taskView->storage()); // add 7 minutes
    }

    return taskView;
}

QString readTextFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        qFatal(QStringLiteral("failed to open file: %1").arg(path).toStdString().c_str());
    }

    QTextStream in(&file);
    return in.readAll();
}
