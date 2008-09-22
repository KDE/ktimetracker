/*
 *     Copyright (C) 2007 by Mathias Soeken <msoeken@tzi.de>
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
  kDebug(5970) << "Entering function";
  if (mWidget)
  {
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
  kDebug(5970) << "Entering function";
  qDeleteAll( mActions );
}

void TreeViewHeaderContextMenu::slotCustomContextMenuRequested( const QPoint& pos ) 
{
  kDebug(5970) << "Entering function";
  if (mWidget && mContextMenu)
  {
    mContextMenu->exec( mWidget->mapToGlobal(pos) );
  }
}

void TreeViewHeaderContextMenu::updateActions() 
{
  kDebug(5970) << "Entering function";
  if (mWidget)
  {
    QAction *action;
    foreach (action, mActions)
    {
      mContextMenu->removeAction( action );
    }

    mActionColumnMapping.clear();
    qDeleteAll( mActions );
    mActions.clear();

    for (int c = 0; c < mWidget->model()->columnCount(); ++c)
    {
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
  kDebug(5970) << "Entering function";
  if (mWidget && action)
  {
    int column = mActionColumnMapping[action];
    bool hidden = mWidget->isColumnHidden(column);
    mWidget->setColumnHidden( column, !hidden );
    updateAction( action, column );
    emit columnToggled( column );
  }
}

void TreeViewHeaderContextMenu::slotAboutToShow()
{
  kDebug(5970) << "Entering function";
  QAction *action;
  foreach (action, mActions)
  {
    updateAction( action, mActionColumnMapping[action] );
  }
}

void TreeViewHeaderContextMenu::updateAction( QAction *action, int column )
{
  kDebug(5970) << "Entering function";
  QString text = mWidget->model()->headerData(column, Qt::Horizontal).toString(); 
  switch (mStyle)
  {
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
      action->setText( (mWidget->isColumnHidden(column) ? i18n("Show") : i18n("Hide")) + ' ' + text );
      break;
  }
}

#include "treeviewheadercontextmenu.moc"
