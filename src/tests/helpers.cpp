/*
 * Copyright (c) 2019 Alexander Potashev <aspotashev@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "helpers.h"

#include <QTemporaryFile>
#include <QTextStream>

#include "taskview.h"
#include "model/task.h"

TaskView *createTaskView(QObject *parent, bool simpleTree)
{
    auto *taskView = new TaskView();
    auto *icsFile = new QTemporaryFile(parent);
    if (!icsFile->open()) {
        delete taskView;
        delete icsFile;
        return nullptr;
    }

    taskView->load(QUrl::fromLocalFile(icsFile->fileName()));

    if (simpleTree) {
        Task* task1 = taskView->task(taskView->addTask("1"));
        Task* task2 = taskView->task(taskView->addTask("2", QString(), 0, 0, QVector<int>(0, 0), task1));
        Task* task3 = taskView->task(taskView->addTask("3"));

        task1->changeTime(5, taskView->storage()->eventsModel()); // add 5 minutes
        task2->changeTime(3, taskView->storage()->eventsModel()); // add 3 minutes
        task3->changeTime(7, taskView->storage()->eventsModel()); // add 7 minutes
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
