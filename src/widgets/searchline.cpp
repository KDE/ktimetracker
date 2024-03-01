/*
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "searchline.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLineEdit>
#include <QTimer>

#include <KLocalizedString>

SearchLine::SearchLine(QWidget *parent)
    : QWidget(parent)
    , m_lineEdit(new QLineEdit(this))
    , m_queuedSearches(0)
    , m_queuedSearchText()
{
    m_lineEdit->setPlaceholderText(i18nc("@info:placeholder", "Search or add task"));
    m_lineEdit->setWhatsThis(i18nc("@info:whatsthis",
                                   "This is a combined field. As long as you do not press Enter, it "
                                   "acts as a filter. "
                                   "Then, only tasks that match your input are shown. "
                                   "As soon as you press Enter, your input is used as name to "
                                   "create a new task."));
    m_lineEdit->installEventFilter(this);
    connect(m_lineEdit, &QLineEdit::textChanged, this, &SearchLine::queueSearch);

    QLayout *innerLayout = new QHBoxLayout;
    innerLayout->addWidget(m_lineEdit);

    setLayout(innerLayout);
}

bool SearchLine::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_lineEdit && event->type() == QEvent::KeyPress) {
        auto *keyEvent = dynamic_cast<QKeyEvent *>(event);
        if (keyEvent && (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return)) {
            if (!m_lineEdit->displayText().isEmpty()) {
                Q_EMIT textSubmitted(m_lineEdit->displayText());
                m_lineEdit->clear();
            }

            return true;
        }
    }

    return QObject::eventFilter(obj, event);
}

void SearchLine::queueSearch(const QString &text)
{
    m_queuedSearches++;
    m_queuedSearchText = text;

    QTimer::singleShot(200, this, &SearchLine::activateSearch);
}

void SearchLine::activateSearch()
{
    m_queuedSearches--;

    if (m_queuedSearches == 0) {
        Q_EMIT searchUpdated(m_queuedSearchText);
    }
}

#include "moc_searchline.cpp"
