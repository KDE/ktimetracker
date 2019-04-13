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
#include <QDebug>

#include <KLocalizedString>

#include "ktt_debug.h"

TreeViewHeaderContextMenu::TreeViewHeaderContextMenu(QObject* parent, QTreeView* widget, int style, QVector<int> &&excludedColumns)
    : QObject(parent)
    , mWidget(widget)
    , mContextMenu(nullptr)
    , mStyle(style)
    , mExcludedColumns(excludedColumns)
{
    if (mWidget) {
        mWidget->header()->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(mWidget->header(), &QHeaderView::customContextMenuRequested, this, &TreeViewHeaderContextMenu::slotCustomContextMenuRequested);

        mContextMenu = new QMenu(mWidget);
        mContextMenu->addSection(i18n("Columns"));
        connect( mContextMenu, SIGNAL(triggered(QAction*)), this, SLOT(slotTriggered(QAction*)) );
        connect( mContextMenu, SIGNAL(aboutToShow()), this, SLOT(slotAboutToShow()) );
        updateActions();
    }
}

TreeViewHeaderContextMenu::~TreeViewHeaderContextMenu() 
{
    qDeleteAll(mActions);
}

void TreeViewHeaderContextMenu::slotCustomContextMenuRequested(const QPoint& pos)
{
    qCDebug(KTT_LOG) << "Entering function";
    if (mWidget && mContextMenu) {
        mContextMenu->exec(mWidget->mapToGlobal(pos));
    }
}

void TreeViewHeaderContextMenu::updateActions() 
{
    qCDebug(KTT_LOG) << "Entering function";
    if (mWidget) {
        for (QAction *action : mActions) {
            mContextMenu->removeAction(action);
        }

        mActionColumnMapping.clear();
        qDeleteAll(mActions);
        mActions.clear();

        for (int c = 0; c < mWidget->model()->columnCount(); ++c) {
            if (mExcludedColumns.contains(c)) {
                continue;
            }

            auto* action = new QAction(this);
            updateAction(action, c);
            mActions.append(action);

            mContextMenu->addAction(action);
            mActionColumnMapping[action] = c;
        }
    }
}

void TreeViewHeaderContextMenu::slotTriggered(QAction* action)
{
    qCDebug(KTT_LOG) << "Entering function";
    if (mWidget && action) {
        int column = mActionColumnMapping[action];
        bool hidden = mWidget->isColumnHidden(column);
        mWidget->setColumnHidden(column, !hidden);
        updateAction(action, column);
        emit columnToggled(column);
    }
}

void TreeViewHeaderContextMenu::slotAboutToShow()
{
    qCDebug(KTT_LOG) << "Entering function";
    for (QAction *action : mActions) {
        updateAction(action, mActionColumnMapping[action]);
    }
}

void TreeViewHeaderContextMenu::updateAction(QAction *action, int column)
{
    qCDebug(KTT_LOG) << "Entering function";
    QString text = mWidget->model()->headerData(column, Qt::Horizontal).toString();
    switch (mStyle)
    {
        case AlwaysCheckBox:
            action->setCheckable(true);
            action->setChecked(!mWidget->isColumnHidden(column));
            action->setText(text);
            break;
        case CheckBoxOnChecked:
            action->setCheckable(!mWidget->isColumnHidden(column));
            action->setChecked(!mWidget->isColumnHidden(column));
            action->setText(text);
            break;
        case ShowHideText:
            action->setCheckable( false );
            action->setChecked( false );
            action->setText((mWidget->isColumnHidden(column) ? i18n("Show") : i18n("Hide")) + ' ' + text);
            break;
    }
}
