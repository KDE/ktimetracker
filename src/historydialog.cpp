/*
 * Copyright (C) 2007 by Thorsten Staerk and Mathias Soeken <msoeken@tzi.de>
 * Copyright (C) 2019  Alexander Potashev <aspotashev@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the
 *      Free Software Foundation, Inc.
 *      51 Franklin Street, Fifth Floor
 *      Boston, MA  02110-1301  USA.
 *
 */

#include "historydialog.h"

#include <QItemDelegate>
#include <QDateTimeEdit>

#include <KMessageBox>

#include "file/filecalendar.h"
#include "model/event.h"
#include "model/eventsmodel.h"
#include "model/projectmodel.h"
#include "model/tasksmodel.h"
#include "model/task.h"
#include "ktt_debug.h"

static const QString dateTimeFormat = QStringLiteral("yyyy-MM-dd HH:mm:ss");

class HistoryWidgetDelegate : public QItemDelegate
{
public:
    explicit HistoryWidgetDelegate(QObject *parent = nullptr)
        : QItemDelegate(parent)
    {
    }

    QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &) const override
    {
        auto* editor = new QDateTimeEdit(parent);
        editor->setAutoFillBackground(true);
        editor->setPalette(option.palette);
        editor->setBackgroundRole(QPalette::Background);
        return editor;
    }

    void setEditorData(QWidget *editor, const QModelIndex &index) const override
    {
        QDateTime dateTime = QDateTime::fromString(index.model()->data(index, Qt::DisplayRole).toString(), dateTimeFormat);
        auto *dateTimeWidget = dynamic_cast<QDateTimeEdit*>(editor);
        if (dateTimeWidget) {
            dateTimeWidget->setDateTime(dateTime);
        } else {
            qCWarning(KTT_LOG) << "Cast to QDateTimeEdit failed";
        }
    }

    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override
    {
        auto *dateTimeWidget = dynamic_cast<QDateTimeEdit*>(editor);
        if (dateTimeWidget) {
            QDateTime dateTime = dateTimeWidget->dateTime();
            model->setData(index, dateTime.toString(dateTimeFormat), Qt::EditRole);
        } else {
            qCWarning(KTT_LOG) << "Cast to QDateTimeEdit failed";
        }
    }

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &) const override
    {
        editor->setGeometry(option.rect);
    }
};

HistoryDialog::HistoryDialog(QWidget *parent, ProjectModel *projectModel)
    : QDialog(parent)
    , m_projectModel(projectModel)
    , m_ui()
{
    m_ui.setupUi(this);
    /* Item Delegate for displaying KDateTimeWidget instead of QLineEdit */
    auto *historyWidgetDelegate = new HistoryWidgetDelegate(m_ui.historytablewidget);
    m_ui.historytablewidget->setItemDelegateForColumn(1, historyWidgetDelegate);
    m_ui.historytablewidget->setItemDelegateForColumn(2, historyWidgetDelegate);

    m_ui.historytablewidget->setEditTriggers(QAbstractItemView::AllEditTriggers);
    m_ui.historytablewidget->setColumnCount(5);
    m_ui.historytablewidget->setHorizontalHeaderLabels(
        QStringList{i18n("Task"), i18n("StartTime"), i18n("EndTime"), i18n("Comment"), QStringLiteral("event UID")});
    m_ui.historytablewidget->horizontalHeader()->setStretchLastSection(true);
    m_ui.historytablewidget->setColumnHidden(4, true);  // hide the "UID" column
    listAllEvents();
    m_ui.historytablewidget->setSortingEnabled(true);
    m_ui.historytablewidget->sortItems(1, Qt::DescendingOrder);
    m_ui.historytablewidget->resizeColumnsToContents();
}

QString HistoryDialog::listAllEvents()
{
    QString err = QString();
    // if sorting is enabled and we write to row x, we cannot be sure row x will be in row x some lines later
    bool old_sortingenabled = m_ui.historytablewidget->isSortingEnabled();
    m_ui.historytablewidget->setSortingEnabled(false);
    connect(m_ui.historytablewidget, &QTableWidget::cellChanged, this, &HistoryDialog::onCellChanged);

    for (const auto *event : m_projectModel->eventsModel()->events()) {
        int row = m_ui.historytablewidget->rowCount();
        m_ui.historytablewidget->insertRow(row);

        // maybe the file is corrupt and (*i)->relatedTo is NULL
        if (event->relatedTo().isEmpty()) {
            qCDebug(KTT_LOG) << "There is no 'relatedTo' entry for " << event->summary();
            err = "NoRelatedToForEvent";
            continue;
        }

        const Task *parent = dynamic_cast<Task*>(m_projectModel->tasksModel()->taskByUID(event->relatedTo()));
        if (!parent) {
            qFatal("orphan event");
        }

        auto *item = new QTableWidgetItem(parent->name());
        item->setFlags(Qt::ItemIsEnabled);
        item->setWhatsThis(i18n("You can change this task's comment, start time and end time."));
        m_ui.historytablewidget->setItem(row, 0, item);

        QDateTime start = event->dtStart();
        QDateTime end = event->dtEnd();
        m_ui.historytablewidget->setItem(row, 1, new QTableWidgetItem(start.toString(dateTimeFormat)));
        m_ui.historytablewidget->setItem(row, 2, new QTableWidgetItem(end.toString(dateTimeFormat)));
        m_ui.historytablewidget->setItem(row, 4, new QTableWidgetItem(event->uid()));
        qDebug() << "event->comments.count() =" << event->comments().count();
        if (event->comments().count() > 0) {
            m_ui.historytablewidget->setItem(row, 3, new QTableWidgetItem(event->comments().last()));
        }
    }

    m_ui.historytablewidget->resizeColumnsToContents();
    m_ui.historytablewidget->setColumnWidth(1, 300);
    m_ui.historytablewidget->setColumnWidth(2, 300);
    setMinimumSize(
        m_ui.historytablewidget->columnWidth(0) +
        m_ui.historytablewidget->columnWidth(1) +
        m_ui.historytablewidget->columnWidth(2) +
        m_ui.historytablewidget->columnWidth(3), height());
    m_ui.historytablewidget->setSortingEnabled(old_sortingenabled);
    return err;
}

void HistoryDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
        case QEvent::LanguageChange:
            m_ui.retranslateUi(this);
            break;
        default:
            break;
    }
}

void HistoryDialog::onCellChanged(int row, int col)
{
    qCDebug(KTT_LOG) << "row =" << row << " col =" << col;

    if (!m_ui.historytablewidget->item(row, 4)) {
        // the program did the change, not the user
        return;
    }

    switch (col) {
        case 1: {
            // StartDate changed
            qCDebug(KTT_LOG) << "user changed StartDate to" << m_ui.historytablewidget->item(row, col)->text();
            QDateTime datetime = QDateTime::fromString(m_ui.historytablewidget->item(row, col)->text(), dateTimeFormat);
            if (!datetime.isValid()) {
                KMessageBox::information(nullptr, i18n("This is not a valid Date/Time."));
                break;
            }

            QString uid = m_ui.historytablewidget->item(row, 4)->text();
            Event *event = m_projectModel->eventsModel()->eventByUID(uid);

            event->setDtStart(datetime);
            // setDtStart could modify date/time, sync it into our table
            m_ui.historytablewidget->item(row, col)->setText(event->dtStart().toString(dateTimeFormat));

            emit m_projectModel->eventsModel()->timesChanged();
            qCDebug(KTT_LOG) << "Program SetDtStart to" << m_ui.historytablewidget->item(row, col)->text();
            break;
        }
        case 2: {
            // EndDate changed
            qCDebug(KTT_LOG) << "user changed EndDate to" << m_ui.historytablewidget->item(row,col)->text();
            QDateTime datetime = QDateTime::fromString(m_ui.historytablewidget->item(row, col)->text(), dateTimeFormat);
            if (!datetime.isValid()) {
                KMessageBox::information(nullptr, i18n("This is not a valid Date/Time."));
                break;
            }

            QString uid = m_ui.historytablewidget->item(row, 4)->text();
            Event *event = m_projectModel->eventsModel()->eventByUID(uid);

            event->setDtEnd(datetime);
            // setDtStart could modify date/time, sync it into our table
            m_ui.historytablewidget->item(row, col)->setText(event->dtEnd().toString(dateTimeFormat));

            emit m_projectModel->eventsModel()->timesChanged();
            qCDebug(KTT_LOG) << "Program SetDtEnd to" << m_ui.historytablewidget->item(row, col)->text();
            break;
        }
        case 3: {
            // Comment changed
            qCDebug(KTT_LOG) << "user changed Comment to" << m_ui.historytablewidget->item(row, col)->text();

            QString uid = m_ui.historytablewidget->item(row, 4)->text();
            Event *event = m_projectModel->eventsModel()->eventByUID(uid);
            qCDebug(KTT_LOG) << "uid =" << uid;
            event->addComment(m_ui.historytablewidget->item(row, col)->text());
            qCDebug(KTT_LOG) << "added" << m_ui.historytablewidget->item(row, col)->text();
            break;
        }
        default:
            break;
    }
}

QString HistoryDialog::refresh()
{
    while (m_ui.historytablewidget->rowCount() > 0) {
        m_ui.historytablewidget->removeRow(0);
    }
    listAllEvents();
    return QString();
}

void HistoryDialog::on_deletepushbutton_clicked()
{
    if (m_ui.historytablewidget->item(m_ui.historytablewidget->currentRow(), 4)) {
        // if an item is current
        QString uid = m_ui.historytablewidget->item(m_ui.historytablewidget->currentRow(), 4)->text();
        qDebug() << "uid =" << uid;
        const Event *event = m_projectModel->eventsModel()->eventByUID(uid);
        if (event) {
            qCDebug(KTT_LOG) << "removing uid " << event->uid();
            m_projectModel->eventsModel()->removeByUID(event->uid());
            emit m_projectModel->eventsModel()->timesChanged();
            this->refresh();
        }
    } else {
        KMessageBox::information(this, i18n("Please select a task to delete."));
    }
}

void HistoryDialog::on_okpushbutton_clicked()
{
    m_ui.historytablewidget->setCurrentCell(0, 0); // you need to change the cell to store the value
    close();
}
