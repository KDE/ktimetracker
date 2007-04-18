#include "treeviewheadercontextmenu.h"

#include <QAction>
#include <QTreeView>
#include <QHeaderView>
#include <KMenu>

#include <KDebug>
#include <KLocale>

TreeViewHeaderContextMenu::TreeViewHeaderContextMenu( QObject *parent, QTreeView *widget, int style, QVector<int> excludedColumns )
  : QObject( parent ),
    mWidget( widget ),
    mContextMenu( 0 ),
    mStyle( style ),
    mExcludedColumns( excludedColumns )
{
  if (mWidget) {
    mWidget->header()->setContextMenuPolicy( Qt::CustomContextMenu );
    connect( mWidget->header(), SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(slotCustomContextMenuRequested(const QPoint&)) );

    mContextMenu = new KMenu( mWidget );
    mContextMenu->addTitle( i18n("Columns") );
    connect( mContextMenu, SIGNAL(triggered(QAction*)), this, SLOT(slotTriggered(QAction*)) );
    connect( mContextMenu, SIGNAL(aboutToShow()), this, SLOT(slotAboutToShow()) );
    updateActions();
  }
}

TreeViewHeaderContextMenu::~TreeViewHeaderContextMenu() 
{
  qDeleteAll( mActions );
}

void TreeViewHeaderContextMenu::slotCustomContextMenuRequested( const QPoint& pos ) 
{
  if (mWidget && mContextMenu) {
    mContextMenu->exec( mWidget->mapToGlobal(pos) );
  }
}

void TreeViewHeaderContextMenu::updateActions() 
{
  if (mWidget) {
    QAction *action;
    foreach (action, mActions) {
      mContextMenu->removeAction( action );
    }

    mActionColumnMapping.clear();
    qDeleteAll( mActions );
    mActions.clear();

    for (int c = 0; c < mWidget->model()->columnCount(); ++c) {
      if (mExcludedColumns.contains( c )) continue;

      QAction* action = new QAction( this );
      updateAction( action, c );
      mActions.append( action );

      mContextMenu->addAction( action );
      mActionColumnMapping[action] = c;
    }
  }
}

void TreeViewHeaderContextMenu::slotTriggered( QAction *action )
{
  if (mWidget && action) {
    int column = mActionColumnMapping[action];
    bool hidden = mWidget->isColumnHidden(column);
    mWidget->setColumnHidden( column, !hidden );
    updateAction( action, column );
    emit columnToggled( column );
  }
}

void TreeViewHeaderContextMenu::slotAboutToShow()
{
  QAction *action;
  foreach (action, mActions) {
    updateAction( action, mActionColumnMapping[action] );
  }
}

void TreeViewHeaderContextMenu::updateAction( QAction *action, int column )
{
  QString text = mWidget->model()->headerData(column, Qt::Horizontal).toString(); 
  switch (mStyle) {
    case AlwaysCheckBox:
      action->setCheckable( true );
      action->setChecked( !mWidget->isColumnHidden(column) );
      action->setText( text );
      break;
    case CheckBoxOnChecked:
      action->setCheckable( !mWidget->isColumnHidden(column) );
      action->setChecked( !mWidget->isColumnHidden(column) );
      action->setText( text );
      break;
    case ShowHideText:
      action->setCheckable( false );
      action->setChecked( false );
      action->setText( (mWidget->isColumnHidden(column) ? i18n("Show") : i18n("Hide")) + " " + text );
      break;
  }
}

#include "treeviewheadercontextmenu.cpp.moc"
