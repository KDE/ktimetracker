/*
 *     Copyright (C) 2007 the ktimetracker developers
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

#include <QHeaderView>
#include <QItemDelegate>
#include <QModelIndex>
#include <QStyleOptionViewItem>
#include <QTableWidget>

#include <KDateTimeWidget>
#include <KDebug>
#include <KLocale>
#include <KMessageBox>

#include "taskview.h"

#include "edithistorydialog.h"

class HistoryWidgetDelegate : public QItemDelegate {
public:
  HistoryWidgetDelegate( QObject *parent = 0 ) : QItemDelegate( parent ) {}

  QWidget *createEditor( QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &) const {
    KDateTimeWidget *editor = new KDateTimeWidget( parent );
    editor->setAutoFillBackground( true );
    editor->setPalette( option.palette );
    // FIXME highlight() background
    editor->setBackgroundRole( QPalette::Background );

    return editor;
  }

  void setEditorData( QWidget *editor, const QModelIndex &index ) const {
    QDateTime dateTime = QDateTime::fromString( index.model()->data( index, Qt::DisplayRole ).toString(), "yyyy-MM-dd HH:mm:ss" );

    KDateTimeWidget *dateTimeWidget = static_cast<KDateTimeWidget*>( editor );
    dateTimeWidget->setDateTime( dateTime );
  }

  void setModelData( QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const {
    KDateTimeWidget *dateTimeWidget = static_cast<KDateTimeWidget*>( editor );
    QDateTime dateTime = dateTimeWidget->dateTime();

    model->setData( index, dateTime.toString( "yyyy-MM-dd HH:mm:ss" ), Qt::EditRole );
  }

  void updateEditorGeometry( QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &) const {
    editor->setGeometry( option.rect );
  }
};

EditHistoryDialog::EditHistoryDialog( TaskView *parent ) 
  : KDialog( parent, Qt::WindowContextHelpButtonHint ),
    mParent( parent )
{
  setButtons( KDialog::Close );
  setWindowTitle( i18n( "Edit History" ) );

  mHistoryWidget = new QTableWidget( this );

  /* Item Delegate for displaying KDateTimeWidget instead of QLineEdit */
  HistoryWidgetDelegate *historyWidgetDelegate = new HistoryWidgetDelegate( this );
  mHistoryWidget->setItemDelegateForColumn( 1, historyWidgetDelegate );
  mHistoryWidget->setItemDelegateForColumn( 2, historyWidgetDelegate );

  /* Columns */
  mHistoryWidget->setColumnCount( 5 );
  mHistoryWidget->setEditTriggers( QAbstractItemView::AllEditTriggers );
  mHistoryWidget->setHorizontalHeaderLabels( 
    QStringList() << i18n( "Task" ) << i18n( "StartTime" ) << i18n( "EndTime" )
                  << i18n( "Comment" ) );
  mHistoryWidget->horizontalHeader()->setStretchLastSection( true );
  mHistoryWidget->setColumnHidden( 4, true );  // hide the "UID" column
  listAllEvents();

  setMainWidget( mHistoryWidget );
}

void EditHistoryDialog::listAllEvents()
{
  connect( mHistoryWidget, SIGNAL( cellChanged( int, int ) ), 
           this, SLOT( historyWidgetCellChanged( int, int ) ) );

  KCal::Event::List eventList = mParent->storage()->rawevents();
  for ( KCal::Event::List::iterator i = eventList.begin(); 
        i != eventList.end(); ++i ) {
    int row = mHistoryWidget->rowCount();
    mHistoryWidget->insertRow( row );
    QTableWidgetItem* item = new QTableWidgetItem( (*i)->relatedTo()->summary() );
    item->setFlags( Qt::ItemIsEnabled );
    item->setWhatsThis( i18n( "You can change this task's comment, start time and end time." ) );
    mHistoryWidget->setItem( row, 0, item );
    QDateTime datetime = QDateTime::fromString( (*i)->dtStart().toString(), Qt::ISODate );
    kDebug() << datetime << endl;
    QDateTime datetime2 = QDateTime::fromString( (*i)->dtEnd().toString(),Qt::ISODate );
    mHistoryWidget->setItem( row, 1, new QTableWidgetItem( datetime.toString( "yyyy-MM-dd HH:mm:ss" ) ) );
    mHistoryWidget->setItem( row, 2, new QTableWidgetItem( datetime2.toString( "yyyy-MM-dd HH:mm:ss" ) ) );
    mHistoryWidget->setItem( row, 4, new QTableWidgetItem( (*i)->uid() ) );
    kDebug() << "(*i)->comments.count() ="  << (*i)->comments().count() << endl;
    if ( (*i)->comments().count() > 0 ) {
      mHistoryWidget->setItem( row, 3, new QTableWidgetItem( (*i)->comments().last() ) );
    }
  }
  mHistoryWidget->resizeColumnsToContents();
  mHistoryWidget->setColumnWidth( 1, 300 );
  mHistoryWidget->setColumnWidth( 2, 300 );
  setMinimumSize( mHistoryWidget->columnWidth( 0 ) 
                        + mHistoryWidget->columnWidth( 1 ) 
                        + mHistoryWidget->columnWidth( 2 ) 
                        + mHistoryWidget->columnWidth( 3 ), height() );
}

void EditHistoryDialog::historyWidgetCellChanged( int row, int col )
{
  kDebug( 5970 ) << "Entering mHistoryWidgetchanged" << endl;
  kDebug( 5970 ) << "row =" << row << " col =" << col << endl;
  if ( mHistoryWidget->item( row, 4 ) ) { // the user did the change, not the program
    if ( col == 1 ) { // StartDate changed
      kDebug( 5970 ) << "user changed StartDate to " << mHistoryWidget->item( row, col )->text() << endl;
      QString uid = mHistoryWidget->item( row, 4 )->text();
      kDebug() << "uid = " << uid << endl;
      KCal::Event::List eventList = mParent->storage()->rawevents();
      for( KCal::Event::List::iterator i = eventList.begin(); i != eventList.end(); ++i ) {
        kDebug() << "row=" << row << " col=" << col << endl;
        if ( (*i)->uid() == uid ) {
          if ( KDateTime::fromString( mHistoryWidget->item( row, col )->text() ).isValid() ) {
            QDateTime datetime = QDateTime::fromString( mHistoryWidget->item( row, col )->text(), "yyyy-MM-dd HH:mm:ss" );
            KDateTime kdatetime = KDateTime::fromString( datetime.toString( Qt::ISODate ) );
            (*i)->setDtStart( kdatetime );
            kDebug() << "Program SetDtStart to " << mHistoryWidget->item( row, col )->text() << endl;
          }
          else 
            KMessageBox::information( 0, i18n( "This is not a valid Date/Time." ) );
        }
      }
    }
    if ( col == 2 ) { // EndDate changed
      kDebug( 5970 ) << "user changed EndDate to " << mHistoryWidget->item(row,col)->text() << endl;
      QString uid = mHistoryWidget->item( row, 4 )->text();
      kDebug() << "uid = " << uid << endl;
      KCal::Event::List eventList = mParent->storage()->rawevents();
      for( KCal::Event::List::iterator i = eventList.begin(); i != eventList.end(); ++i) {
        kDebug() << "row=" << row << " col=" << col << endl;
        if ( (*i)->uid() == uid ) {
          if ( KDateTime::fromString( mHistoryWidget->item( row, col )->text() ).isValid() ) {
            QDateTime datetime = QDateTime::fromString( mHistoryWidget->item( row, col )->text(), "yyyy-MM-dd HH:mm:ss" );
            KDateTime kdatetime = KDateTime::fromString( datetime.toString( Qt::ISODate ) );
            (*i)->setDtEnd( kdatetime ); 
            kDebug() << "Program SetDtEnd to " << mHistoryWidget->item( row, col )->text() << endl;
          }
          else 
            KMessageBox::information( 0, i18n( "This is not a valid Date/Time." ) );
        }
      }
    }
    if ( col == 3 ) { // Comment changed
      kDebug( 5970 ) << "user changed Comment to " << mHistoryWidget->item(row,col)->text() << endl;
      QString uid = mHistoryWidget->item( row, 4 )->text();
      kDebug() << "uid = " << uid << endl;
      KCal::Event::List eventList = mParent->storage()->rawevents();
      for ( KCal::Event::List::iterator i = eventList.begin(); i != eventList.end(); ++i ) {
        kDebug() << "row=" << row << " col=" << col << endl;
        if ( (*i)->uid() == uid ) {
          (*i)->addComment( mHistoryWidget->item( row, col )->text() ); 
          kDebug() << "added " << mHistoryWidget->item( row, col )->text() << endl;
        }
      }
    }
  }
}

#include "edithistorydialog.moc"
