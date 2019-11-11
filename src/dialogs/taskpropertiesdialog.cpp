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

#include "taskpropertiesdialog.h"

#include <QGridLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QLabel>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QScrollArea>
#include <QCheckBox>

#include <KLocalizedString>
#include <KPluralHandlingSpinBox>
#include <KWindowSystem>

#include "ktimetrackerutility.h"

TaskPropertiesDialog::TaskPropertiesDialog(
    QWidget *parent, const QString &caption,
    const QString &name, const QString &description,
    const DesktopList &desktops)
    : QDialog(parent),
      m_nameText(nullptr),
      m_descText(nullptr),
      m_trackingDesktops()
{
    setWindowTitle(caption);
    setModal(true);
    setMinimumSize(500, 330);

    auto *mainLayout = new QVBoxLayout(this);
    setLayout(mainLayout);

    // Task group
    auto *infoGroup = new QGroupBox(i18nc("@title:group", "Task"), this);
    auto *infoLayout = new QGridLayout(infoGroup);
    infoGroup->setLayout(infoLayout);

    m_nameText = new QLineEdit(name, infoGroup);
    m_nameText->setWhatsThis(i18nc(
        "@info:whatsthis",
        "<p>Enter the name of the task here. You can choose it freely.</p>\n"
        "<p><i>Example:</i> phone with mother</p>"));
    infoLayout->addWidget(m_nameText, 0, 1);
    infoLayout->addWidget(new QLabel(i18n("Task Name:")), 0, 0);

    m_descText = new QPlainTextEdit(description, infoGroup);
    m_descText->setWordWrapMode(QTextOption::WordWrap);
    infoLayout->addWidget(m_descText, 1, 1);
    infoLayout->addWidget(new QLabel(i18n("Task Description:")), 1, 0);

    // Desktop tracking group
    m_trackingGroup = new QGroupBox(i18nc("@title:group", "Auto Tracking"), this);
    m_trackingGroup->setCheckable(true);
    m_trackingGroup->setChecked(!desktops.isEmpty());

    auto *trackingLayout = new QHBoxLayout(m_trackingGroup);
    m_trackingGroup->setLayout(trackingLayout);

    auto *scrollArea = new QScrollArea(m_trackingGroup);

    const int numDesktops = KWindowSystem::numberOfDesktops();

    auto *desktopsWidget = new QWidget(m_trackingGroup);
    auto *desktopsLayout = new QGridLayout(scrollArea);
    desktopsLayout->addItem(new QSpacerItem(50, 0), 0, 2, numDesktops, 1);
    desktopsWidget->setLayout(desktopsLayout);

    for (int i = 0; i < numDesktops; ++i) {
        desktopsLayout->addWidget(new QLabel(i18nc("order number of desktop: 1, 2, ...", "%1.", i + 1)), i, 0);

        auto *checkbox = new QCheckBox(KWindowSystem::desktopName(i + 1));
        desktopsLayout->addWidget(checkbox, i, 1);
        m_trackingDesktops.append(checkbox);
    }

    for (int index : desktops) {
        if (index >= 0 && index < numDesktops) {
            m_trackingDesktops[index]->setChecked(true);
        }
    }

    scrollArea->setWidget(desktopsWidget);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    trackingLayout->addStretch(1);
    trackingLayout->addWidget(scrollArea);
    trackingLayout->addStretch(1);

    auto *m_buttonBox = new QDialogButtonBox(this);
    m_buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &TaskPropertiesDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &TaskPropertiesDialog::reject);

    mainLayout->addWidget(infoGroup);
    mainLayout->addWidget(m_trackingGroup, 0);
    mainLayout->addStretch(0);
    mainLayout->addWidget(m_buttonBox);
}

QString TaskPropertiesDialog::name() const
{
    return m_nameText->text();
}

QString TaskPropertiesDialog::description() const
{
    return m_descText->toPlainText();
}

DesktopList TaskPropertiesDialog::desktops() const
{
    DesktopList res;
    if (m_trackingGroup->isChecked()) {
        int i = 0;
        for (auto *checkbox : m_trackingDesktops) {
            if (checkbox->isChecked()) {
                res.append(i);
            }

            i++;
        }
    }

    return res;
}
