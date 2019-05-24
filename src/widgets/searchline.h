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

#ifndef KTIMETRACKER_SEARCHLINE_H
#define KTIMETRACKER_SEARCHLINE_H

#include <QWidget>

class QLineEdit;

class SearchLine : public QWidget
{
    Q_OBJECT

public:
    explicit SearchLine(QWidget *parent = nullptr);

Q_SIGNALS:
    void textSubmitted(const QString &text);
    void searchUpdated(const QString &text);

private Q_SLOTS:
    void queueSearch(const QString &text);
    void activateSearch();

private:
    bool eventFilter(QObject* obj, QEvent* event) override;

    QLineEdit *m_lineEdit;
    int m_queuedSearches;
    QString m_queuedSearchText;
};

#endif // KTIMETRACKER_SEARCHLINE_H
