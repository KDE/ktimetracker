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

#include "edittimedialog.h"

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>

#include <KLocalizedString>
#include <KPluralHandlingSpinBox>

#include "ktimetrackerutility.h"

EditTimeDialog::EditTimeDialog(
    QWidget *parent,
    const QString &name, const QString &description, int64_t minutes)
    : QDialog(parent)
    , m_initialMinutes(minutes)
    , m_changeMinutes(0)
    , m_editHistoryRequested(false)
{
    setWindowTitle(i18nc("@title:window", "Edit Task Time"));
    setModal(true);
    setMinimumSize(500, 330);

    auto *mainLayout = new QVBoxLayout(this);
    setLayout(mainLayout);

    // Info group
    auto *infoGroup = new QGroupBox(i18nc("@title:group", "Task"), this);
    auto *infoLayout = new QGridLayout(infoGroup);
    infoGroup->setLayout(infoLayout);

    QPalette roPalette;
    roPalette.setColor(QPalette::Base, palette().color(QPalette::Window));

    auto *nameText = new QLineEdit(name, infoGroup);
    nameText->setReadOnly(true);
    nameText->setPalette(roPalette);
    infoLayout->addWidget(nameText, 0, 1);
    infoLayout->addWidget(new QLabel(i18n("Task Name:")), 0, 0);

    auto *descText = new QPlainTextEdit(description, infoGroup);
    descText->setReadOnly(true);
    descText->setPalette(roPalette);
    descText->setWordWrapMode(QTextOption::WordWrap);
    infoLayout->addWidget(descText, 1, 1);
    infoLayout->addWidget(new QLabel(i18n("Task Description:")), 1, 0);

    // Edit group
    auto *editGroup = new QGroupBox(i18nc("@title:group", "Time Editing"), this);
    auto *editLayout = new QGridLayout(editGroup);
    editGroup->setLayout(editLayout);

    editLayout->setColumnStretch(0, 1);
    editLayout->setColumnStretch(4, 1);

    editLayout->addWidget(new QLabel(formatTime(minutes)), 0, 2); // TODO increment over time while dialog is open
    editLayout->addWidget(new QLabel(i18n("Current Time:")), 0, 1);

    m_timeEditor = new KPluralHandlingSpinBox(editGroup);
    m_timeEditor->setSuffix(ki18ncp("@item:valuesuffix Change Time By: ... minute(s)", " minute", " minutes"));
    m_timeEditor->setRange(-24 * 3600, 24 * 3600);
    m_timeEditor->setMinimumWidth(200);
    m_timeEditor->setFocus();
    connect(m_timeEditor, QOverload<int>::of(&QSpinBox::valueChanged), this, &EditTimeDialog::update);
    editLayout->addWidget(m_timeEditor, 1, 2);
    editLayout->addWidget(new QLabel(i18n("Change Time By:")), 1, 1);

    m_timePreview = new QLabel(editGroup);
    editLayout->addWidget(m_timePreview, 2, 2); // TODO increment over time while dialog is open
    editLayout->addWidget(new QLabel(i18n("Time After Change:")), 2, 1);

    m_buttonBox = new QDialogButtonBox(this);
    m_buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    auto *historyButton = new QPushButton(
        QIcon::fromTheme("document-edit"), i18nc("@action:button", "Edit History..."), m_buttonBox);
    historyButton->setToolTip(i18n("To change this task's time, you have to edit its event history"));
    m_buttonBox->addButton(historyButton, QDialogButtonBox::HelpRole);

    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &EditTimeDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::helpRequested, [=]() {
        m_editHistoryRequested = true;
        accept();
    });
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &EditTimeDialog::reject);

    mainLayout->addWidget(infoGroup);
    mainLayout->addWidget(editGroup);
    mainLayout->addStretch(1);
    mainLayout->addWidget(m_buttonBox);

    update(0);
}

void EditTimeDialog::update(int newValue)
{
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(newValue != 0);
    m_timePreview->setText(formatTime(m_initialMinutes + newValue));
    m_changeMinutes = newValue;
}
