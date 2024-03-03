/*
    SPDX-FileCopyrightText: 2007 Mathias Soeken <msoeken@tzi.de>
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
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
