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
