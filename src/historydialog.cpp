/*
 *     Copyright (C) 2007 by Thorsten Staerk and Mathias Soeken <msoeken@tzi.de>
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
#include "taskview.h"
#include "kttcalendar.h"

#include <QItemDelegate>
#include <QDateTimeEdit>
#include <KMessageBox>
#include <QDebug>
#include "ktt_debug.h"

class HistoryWidgetDelegate : public QItemDelegate
{
public:
    explicit HistoryWidgetDelegate(QObject *parent = nullptr)
        : QItemDelegate(parent) {}

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
        QDateTime dateTime = QDateTime::fromString( index.model()->data( index, Qt::DisplayRole ).toString(), "yyyy-MM-dd HH:mm:ss" );
        auto* dateTimeWidget = dynamic_cast<QDateTimeEdit*>(editor);
        if (dateTimeWidget) {
            dateTimeWidget->setDateTime(dateTime);
        } else {
            qCWarning(KTT_LOG) << "Cast to QDateTimeEdit failed";
        }
    }

    void setModelData( QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override
    {
        auto* dateTimeWidget = dynamic_cast<QDateTimeEdit*>(editor);
        if (dateTimeWidget) {
            QDateTime dateTime = dateTimeWidget->dateTime();
            model->setData(index, dateTime.toString( "yyyy-MM-dd HH:mm:ss" ), Qt::EditRole);
        } else {
            qCWarning(KTT_LOG) << "Cast to QDateTimeEdit failed";
        }
    }

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &) const override
    {
        editor->setGeometry(option.rect);
    }
};

HistoryDialog::HistoryDialog(TaskView *parent)
    : QDialog(parent)
    , m_ui()
{
    mparent = parent;
    m_ui.setupUi(this);
    /* Item Delegate for displaying KDateTimeWidget instead of QLineEdit */
    auto* historyWidgetDelegate = new HistoryWidgetDelegate(m_ui.historytablewidget);
    m_ui.historytablewidget->setItemDelegateForColumn(1, historyWidgetDelegate);
    m_ui.historytablewidget->setItemDelegateForColumn(2, historyWidgetDelegate);

    m_ui.historytablewidget->setEditTriggers(QAbstractItemView::AllEditTriggers);
    m_ui.historytablewidget->setColumnCount(5);
    m_ui.historytablewidget->setHorizontalHeaderLabels(
    QStringList() << i18n("Task") << i18n("StartTime") << i18n("EndTime")
                  << i18n("Comment") << QString("event UID"));
    m_ui.historytablewidget->horizontalHeader()->setStretchLastSection(true);
    m_ui.historytablewidget->setColumnHidden(4, true);  // hide the "UID" column
    listallevents();
    m_ui.historytablewidget->setSortingEnabled(true);
    m_ui.historytablewidget->sortItems(1, Qt::DescendingOrder);
    m_ui.historytablewidget->resizeColumnsToContents();
}

QString HistoryDialog::listallevents()
{
    QString err=QString();
    // if sorting is enabled and we write to row x, we cannot be sure row x will be in row x some lines later
    bool old_sortingenabled = m_ui.historytablewidget->isSortingEnabled();
    m_ui.historytablewidget->setSortingEnabled( false );
    connect(  m_ui.historytablewidget, SIGNAL(cellChanged(int,int)),
              this, SLOT(historyWidgetCellChanged(int,int)) );

    KCalCore::Event::List eventList = mparent->storage()->rawevents();
    KTTCalendar::Ptr calendar = mparent->storage()->calendar();
    for ( KCalCore::Event::List::iterator i = eventList.begin();
        i != eventList.end(); ++i )
    {
        int row = m_ui.historytablewidget->rowCount();
        m_ui.historytablewidget->insertRow(row);
        QTableWidgetItem* item = nullptr;
        if ( !(*i)->relatedTo().isEmpty() ) { // maybe the file is corrupt and (*i)->relatedTo is NULL
            KCalCore::Incidence::Ptr parent = calendar ? calendar->incidence( (*i)->relatedTo() ) : KCalCore::Incidence::Ptr();
            item = new QTableWidgetItem( parent ? parent->summary() : (*i)->summary() );
            item->setFlags( Qt::ItemIsEnabled );
            item->setWhatsThis( i18n( "You can change this task's comment, start time and end time." ) );
            m_ui.historytablewidget->setItem( row, 0, item );
            // dtStart is stored like DTSTART;TZID=Europe/Berlin:20080327T231056
            // dtEnd is stored like DTEND:20080327T231509Z
            // we need to handle both differently
            QDateTime start = QDateTime::fromTime_t( (*i)->dtStart().toTime_t() );
            QDateTime end = QDateTime::fromString( (*i)->dtEnd().toString(), Qt::ISODate );
            qDebug() << "start =" << start << "; end =" << end;
            m_ui.historytablewidget->setItem( row, 1, new QTableWidgetItem( start.toString( "yyyy-MM-dd HH:mm:ss" ) ) );
            m_ui.historytablewidget->setItem( row, 2, new QTableWidgetItem( end.toString( "yyyy-MM-dd HH:mm:ss" ) ) );
            m_ui.historytablewidget->setItem( row, 4, new QTableWidgetItem( (*i)->uid() ) );
            qDebug() <<"(*i)->comments.count() ="  << (*i)->comments().count();
            if ( (*i)->comments().count() > 0 )
            {
                m_ui.historytablewidget->setItem( row, 3, new QTableWidgetItem( (*i)->comments().last() ) );
            }
        } else {
            qCDebug(KTT_LOG) << "There is no 'relatedTo' entry for " << (*i)->summary();
            err="NoRelatedToForEvent";
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
    switch (e->type())
    {
        case QEvent::LanguageChange:
            m_ui.retranslateUi(this);
            break;
        default:
            break;
    }
}

void HistoryDialog::historyWidgetCellChanged( int row, int col )
{
    qCDebug(KTT_LOG) << "Entering function";
    qCDebug(KTT_LOG) << "row =" << row << " col =" << col;
    if (m_ui.historytablewidget->item(row, 4))
    { // the user did the change, not the program
        if ( col == 1 )
        { // StartDate changed
            qCDebug(KTT_LOG) << "user changed StartDate to" << m_ui.historytablewidget->item( row, col )->text();
            QString uid = m_ui.historytablewidget->item( row, 4 )->text();
            KCalCore::Event::List eventList = mparent->storage()->rawevents();
            for( KCalCore::Event::List::iterator i = eventList.begin(); i != eventList.end(); ++i )
            {
                qCDebug(KTT_LOG) << "row=" << row <<" col=" << col;
                if ( (*i)->uid() == uid )
                {
                    if ( QDateTime::fromString( m_ui.historytablewidget->item( row, col )->text() ).isValid() )
                    {
                        QDateTime datetime = QDateTime::fromString( m_ui.historytablewidget->item( row, col )->text(), "yyyy-MM-dd HH:mm:ss" );
                        (*i)->setDtStart( datetime );
                        mparent->reFreshTimes();
                        qCDebug(KTT_LOG) <<"Program SetDtStart to" << m_ui.historytablewidget->item( row, col )->text();
                    }
                    else
                        KMessageBox::information( 0, i18n( "This is not a valid Date/Time." ) );
                }
            }
        }
        if ( col == 2 )
        { // EndDate changed
            qCDebug(KTT_LOG) <<"user changed EndDate to" << m_ui.historytablewidget->item(row,col)->text();
            QString uid = m_ui.historytablewidget->item( row, 4 )->text();
            KCalCore::Event::List eventList = mparent->storage()->rawevents();
            for( KCalCore::Event::List::iterator i = eventList.begin(); i != eventList.end(); ++i)
            {
                qDebug() <<"row=" << row <<" col=" << col;
                if ( (*i)->uid() == uid )
                {
                    if ( QDateTime::fromString( m_ui.historytablewidget->item( row, col )->text() ).isValid() )
                    {
                        QDateTime datetime = QDateTime::fromString( m_ui.historytablewidget->item( row, col )->text(), "yyyy-MM-dd HH:mm:ss" );
                        (*i)->setDtEnd( datetime );
                        mparent->reFreshTimes();
                        qCDebug(KTT_LOG) <<"Program SetDtEnd to" << m_ui.historytablewidget->item( row, col )->text();
                    }
                    else
                        KMessageBox::information( 0, i18n( "This is not a valid Date/Time." ) );
                }
            }
        }
        if ( col == 3 )
        { // Comment changed
            qCDebug(KTT_LOG) <<"user changed Comment to" << m_ui.historytablewidget->item(row,col)->text();
            QString uid = m_ui.historytablewidget->item( row, 4 )->text();
            qDebug() <<"uid =" << uid;
            KCalCore::Event::List eventList = mparent->storage()->rawevents();
            for ( KCalCore::Event::List::iterator i = eventList.begin(); i != eventList.end(); ++i )
            {
                qDebug() <<"row=" << row <<" col=" << col;
                if ( (*i)->uid() == uid )
                {
                    (*i)->addComment( m_ui.historytablewidget->item( row, col )->text() );
                    qDebug() <<"added" << m_ui.historytablewidget->item( row, col )->text();
                }
            }
        }
    }
}

QString HistoryDialog::refresh()
{
    QString err;
    while (m_ui.historytablewidget->rowCount()>0)
        m_ui.historytablewidget->removeRow(0);
    listallevents();
    return err;
}


void HistoryDialog::on_deletepushbutton_clicked()
{
    if (m_ui.historytablewidget->item( m_ui.historytablewidget->currentRow(), 4))
    { // if an item is current
        QString uid = m_ui.historytablewidget->item( m_ui.historytablewidget->currentRow(), 4 )->text();
        qDebug() <<"uid =" << uid;
        KCalCore::Event::List eventList = mparent->storage()->rawevents();
        for ( KCalCore::Event::List::iterator i = eventList.begin(); i != eventList.end(); ++i )
        {
            if ( (*i)->uid() == uid )
            {
                qCDebug(KTT_LOG) << "removing uid " << (*i)->uid();
                mparent->storage()->removeEvent((*i)->uid());
                mparent->reFreshTimes();
                this->refresh();
            }
        }
    }
    else KMessageBox::information(this, i18n("Please select a task to delete."));
}

void HistoryDialog::on_okpushbutton_clicked()
{
    m_ui.historytablewidget->setCurrentCell(0,0); // you need to change the cell to store the value
    close();
}
