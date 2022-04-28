/*
 * Copyright (C) 2007 by Mathias Soeken <msoeken@tzi.de>
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

#ifndef TREEVIEWHEADERCONTEXTMENU_H
#define TREEVIEWHEADERCONTEXTMENU_H

#include <QHash>
#include <QMenu>
#include <QObject>
#include <QPoint>
#include <QVector>

QT_BEGIN_NAMESPACE
class QAction;
class QTreeView;
QT_END_NAMESPACE

/**
 * ContextMenu for QTreeView::header() to toggle the
 * visible state of the columns.
 *
 * It is possible to exclude columns from inserting in the
 * menu either by the @p excludedColumns parameter in the constructor.
 *
 * @author Mathias Soeken <msoeken@tzi.de>
 */
class TreeViewHeaderContextMenu : public QObject
{
    Q_OBJECT

public:
    TreeViewHeaderContextMenu(QObject *parent, QTreeView *widget, QVector<int> &&excludedColumns);
    ~TreeViewHeaderContextMenu() override;

private Q_SLOTS:
    void slotCustomContextMenuRequested(const QPoint &);

protected Q_SLOTS:
    void updateActions();
    void slotTriggered(QAction *);
    void slotAboutToShow();

Q_SIGNALS:
    void columnToggled(int);

private:
    void updateAction(QAction *action, int column);

    QTreeView *m_widget;
    QVector<QAction *> m_actions;
    QMenu *m_contextMenu;
    QHash<QAction *, int> m_actionColumnMapping;
    QVector<int> m_excludedColumns;
};

#endif
