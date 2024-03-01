/*
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
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
    bool eventFilter(QObject *obj, QEvent *event) override;

    QLineEdit *m_lineEdit;
    int m_queuedSearches;
    QString m_queuedSearchText;
};

#endif // KTIMETRACKER_SEARCHLINE_H
