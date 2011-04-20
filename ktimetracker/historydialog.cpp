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
#include "ui_historydialog.h"
#include "taskview.h"
#include <QItemDelegate>
#include <KDateTimeWidget>
#include <KMessageBox>

class HistoryWidgetDelegate : public QItemDelegate
{

public:
    HistoryWidgetDelegate( QObject *parent = 0 ) : QItemDelegate( parent ) {}

    QWidget *createEditor( QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &) const
    {
        KDateTimeWidget *editor = new KDateTimeWidget( parent );
        editor->setAutoFillBackground( true );
        editor->setPalette( option.palette );
        editor->setBackgroundRole( QPalette::Background );
        return editor;
    }

    void setEditorData( QWidget *editor, const QModelIndex &index ) const
    {
        QDateTime dateTime = QDateTime::fromString( index.model()->data( index, Qt::DisplayRole ).toString(), "yyyy-MM-dd HH:mm:ss" );
        KDateTimeWidget *dateTimeWidget = static_cast<KDateTimeWidget*>( editor );
        dateTimeWidget->setDateTime( dateTime );
    }

    void setModelData( QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
    {
        KDateTimeWidget *dateTimeWidget = static_cast<KDateTimeWidget*>( editor );
        QDateTime dateTime = dateTimeWidget->dateTime();
        model->setData( index, dateTime.toString( "yyyy-MM-dd HH:mm:ss" ), Qt::EditRole );
    }

    void updateEditorGeometry( QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &) const
    {
        editor->setGeometry( option.rect );
    }
};

historydialog::historydialog(TaskView *parent) :
    QDialog(parent),
    m_ui(new Ui::historydialog)
{
    mparent=parent;
    m_ui->setupUi(this);
    /* Item Delegate for displaying KDateTimeWidget instead of QLineEdit */
    HistoryWidgetDelegate *historyWidgetDelegate = new HistoryWidgetDelegate( m_ui->historytablewidget );
    m_ui->historytablewidget->setItemDelegateForColumn( 1, historyWidgetDelegate );
    m_ui->historytablewidget->setItemDelegateForColumn( 2, historyWidgetDelegate );

    m_ui->historytablewidget->setEditTriggers( QAbstractItemView::AllEditTriggers );
    m_ui->historytablewidget->setColumnCount(5);
    m_ui->historytablewidget->setHorizontalHeaderLabels(
    QStringList() << i18n( "Task" ) << i18n( "StartTime" ) << i18n( "EndTime" )
                  << i18n( "Comment" ) << QString( "event UID" ) );
    m_ui->historytablewidget->horizontalHeader()->setStretchLastSection( true );
    m_ui->historytablewidget->setColumnHidden( 4, true );  // hide the "UID" column
    listallevents();
    m_ui->historytablewidget->setSortingEnabled( true );
    m_ui->historytablewidget->sortItems( 1, Qt::DescendingOrder );
    m_ui->historytablewidget->resizeColumnsToContents();
}

historydialog::~historydialog()
{
    delete m_ui;
}

QString historydialog::listallevents()
{
    QString err=QString();
    // if sorting is enabled and we write to row x, we cannot be sure row x will be in row x some lines later
    bool old_sortingenabled=m_ui->historytablewidget->isSortingEnabled();
    m_ui->historytablewidget->setSortingEnabled( false );
    connect(  m_ui->historytablewidget, SIGNAL( cellChanged( int, int ) ),
              this, SLOT( historyWidgetCellChanged( int, int ) ) );

    KCal::Event::List eventList = mparent->storage()->rawevents();
    for ( KCal::Event::List::iterator i = eventList.begin();
        i != eventList.end(); ++i )
    {
        int row =  m_ui->historytablewidget->rowCount();
        m_ui->historytablewidget->insertRow( row );
        QTableWidgetItem* item=0;
        if ( (*i)->relatedTo() ) // maybe the file is corrupt and (*i)->relatedTo is NULL
        {
            item = new QTableWidgetItem( (*i)->relatedTo()->summary() );
            item->setFlags( Qt::ItemIsEnabled );
            item->setWhatsThis( i18n( "You can change this task's comment, start time and end time." ) );
            m_ui->historytablewidget->setItem( row, 0, item );
            // dtStart is stored like DTSTART;TZID=Europe/Berlin:20080327T231056
            // dtEnd is stored like DTEND:20080327T231509Z
            // we need to handle both differently
            QDateTime start = QDateTime::fromTime_t( (*i)->dtStart().toTime_t() );
            QDateTime end = QDateTime::fromString( (*i)->dtEnd().toString(), Qt::ISODate );
            kDebug() << "start =" << start << "; end =" << end;
            m_ui->historytablewidget->setItem( row, 1, new QTableWidgetItem( start.toString( "yyyy-MM-dd HH:mm:ss" ) ) );
            m_ui->historytablewidget->setItem( row, 2, new QTableWidgetItem( end.toString( "yyyy-MM-dd HH:mm:ss" ) ) );
            m_ui->historytablewidget->setItem( row, 4, new QTableWidgetItem( (*i)->uid() ) );
            kDebug() <<"(*i)->comments.count() ="  << (*i)->comments().count();
            if ( (*i)->comments().count() > 0 )
            {
                m_ui->historytablewidget->setItem( row, 3, new QTableWidgetItem( (*i)->comments().last() ) );
            }
        }
        else
        {
            kDebug(5970) << "There is no 'relatedTo' entry for " << (*i)->summary();
            err="NoRelatedToForEvent";
        }
    }
    m_ui->historytablewidget->resizeColumnsToContents();
    m_ui->historytablewidget->setColumnWidth( 1, 300 );
    m_ui->historytablewidget->setColumnWidth( 2, 300 );
    setMinimumSize(  m_ui->historytablewidget->columnWidth( 0 )
                  +  m_ui->historytablewidget->columnWidth( 1 )
                  +  m_ui->historytablewidget->columnWidth( 2 )
                  +  m_ui->historytablewidget->columnWidth( 3 ), height() );
    m_ui->historytablewidget->setSortingEnabled(old_sortingenabled);
    return err;
}

void historydialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type())
    {
        case QEvent::LanguageChange:
            m_ui->retranslateUi(this);
            break;
        default:
            break;
    }
}

void historydialog::historyWidgetCellChanged( int row, int col )
{
    kDebug( 5970 ) << "Entering function";
    kDebug( 5970 ) << "row =" << row << " col =" << col;
    if ( m_ui->historytablewidget->item( row, 4 ) )
    { // the user did the change, not the program
        if ( col == 1 )
        { // StartDate changed
            kDebug( 5970 ) << "user changed StartDate to" << m_ui->historytablewidget->item( row, col )->text();
            QString uid = m_ui->historytablewidget->item( row, 4 )->text();
            KCal::Event::List eventList = mparent->storage()->rawevents();
            for( KCal::Event::List::iterator i = eventList.begin(); i != eventList.end(); ++i )
            {
                kDebug(5970) << "row=" << row <<" col=" << col;
                if ( (*i)->uid() == uid )
                {
                    if ( KDateTime::fromString( m_ui->historytablewidget->item( row, col )->text() ).isValid() )
                    {
                        QDateTime datetime = QDateTime::fromString( m_ui->historytablewidget->item( row, col )->text(), "yyyy-MM-dd HH:mm:ss" );
                        KDateTime kdatetime = KDateTime::fromString( datetime.toString( Qt::ISODate ) );
                        (*i)->setDtStart( kdatetime );
                        mparent->reFreshTimes();
                        kDebug(5970) <<"Program SetDtStart to" << m_ui->historytablewidget->item( row, col )->text();
                    }
                    else
                        KMessageBox::information( 0, i18n( "This is not a valid Date/Time." ) );
                }
            }
        }
        if ( col == 2 )
        { // EndDate changed
            kDebug( 5970 ) <<"user changed EndDate to" << m_ui->historytablewidget->item(row,col)->text();
            QString uid = m_ui->historytablewidget->item( row, 4 )->text();
            KCal::Event::List eventList = mparent->storage()->rawevents();
            for( KCal::Event::List::iterator i = eventList.begin(); i != eventList.end(); ++i)
            {
                kDebug() <<"row=" << row <<" col=" << col;
                if ( (*i)->uid() == uid )
                {
                    if ( KDateTime::fromString( m_ui->historytablewidget->item( row, col )->text() ).isValid() )
                    {
                        QDateTime datetime = QDateTime::fromString( m_ui->historytablewidget->item( row, col )->text(), "yyyy-MM-dd HH:mm:ss" );
                        KDateTime kdatetime = KDateTime::fromString( datetime.toString( Qt::ISODate ) );
                        (*i)->setDtEnd( kdatetime );
                        mparent->reFreshTimes();
                        kDebug(5970) <<"Program SetDtEnd to" << m_ui->historytablewidget->item( row, col )->text();
                    }
                    else
                        KMessageBox::information( 0, i18n( "This is not a valid Date/Time." ) );
                }
            }
        }
        if ( col == 3 )
        { // Comment changed
            kDebug( 5970 ) <<"user changed Comment to" << m_ui->historytablewidget->item(row,col)->text();
            QString uid = m_ui->historytablewidget->item( row, 4 )->text();
            kDebug() <<"uid =" << uid;
            KCal::Event::List eventList = mparent->storage()->rawevents();
            for ( KCal::Event::List::iterator i = eventList.begin(); i != eventList.end(); ++i )
            {
                kDebug() <<"row=" << row <<" col=" << col;
                if ( (*i)->uid() == uid )
                {
                    (*i)->addComment( m_ui->historytablewidget->item( row, col )->text() );
                    kDebug() <<"added" << m_ui->historytablewidget->item( row, col )->text();
                }
            }
        }
    }
}

QString historydialog::refresh()
{
    QString err;
    while (m_ui->historytablewidget->rowCount()>0)
        m_ui->historytablewidget->removeRow(0);
    listallevents();
    return err;
}

#include "historydialog.moc"

void historydialog::on_deletepushbutton_clicked()
{
    if (m_ui->historytablewidget->item( m_ui->historytablewidget->currentRow(), 4))
    { // if an item is current
        QString uid = m_ui->historytablewidget->item( m_ui->historytablewidget->currentRow(), 4 )->text();
        kDebug() <<"uid =" << uid;
        KCal::Event::List eventList = mparent->storage()->rawevents();
        for ( KCal::Event::List::iterator i = eventList.begin(); i != eventList.end(); ++i )
        {
            if ( (*i)->uid() == uid )
            {
                kDebug(5970) << "removing uid " << (*i)->uid();
                mparent->storage()->removeEvent((*i)->uid());
                mparent->reFreshTimes();
                this->refresh();
            }
        }
    }
    else KMessageBox::information(this, i18n("Please select a task to delete."));
}

void historydialog::on_okpushbutton_clicked()
{
    close();
}
