/*
 * Copyright 2019  Alexander Potashev <aspotashev@gmail.com>
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
 *
 */

import QtQuick 2.3
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.11
import QtQuick.Window 2.3
import org.kde.kirigami 2.4 as Kirigami

Window {
    id: dialog
    property int minutes
    property string taskUid
    property string taskName
    property string taskDescription
    property int changeMinutes: 0
    signal changeTime(string taskUid, int minutes)
    signal editHistory()

    visible: false
    modality: Qt.ApplicationModal
    title: i18nc("@title:window", "Edit Task Time")
    minimumWidth: 500
    minimumHeight: 330
    width: minimumWidth
    height: minimumHeight

    onVisibleChanged: {
        if (visible) {
            timeChangeField.focus = true;
            timeChangeField.clear();
        }
    }

    Component.onCompleted: {
        setX(Screen.width / 2 - width / 2);
        setY(Screen.height / 2 - height / 2);
    }

    Kirigami.Page {
        anchors.fill: parent
        focus: true

        ColumnLayout {
            anchors.fill: parent

            GridLayout {
                columns: 2

                Text {
                    text: i18n("Task Name:")
                }

                Text {
                    text: taskName

                    font.bold: true
                    wrapMode: Text.Wrap
                }

                Text {
                    text: i18n("Task Description:")
                }

                Text {
                    text: taskDescription

                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    elide: Text.ElideRight
                    maximumLineCount: 5
                }

                Rectangle {
                    height: 1
                    color: Qt.tint(Kirigami.Theme.textColor, Qt.rgba(Kirigami.Theme.backgroundColor.r, Kirigami.Theme.backgroundColor.g, Kirigami.Theme.backgroundColor.b, 0.7))

                    Layout.columnSpan: 2
                    Layout.fillWidth: true
                    Layout.topMargin: 10
                    Layout.bottomMargin: 10
                }

                Text {
                    text: i18n("Current Time:")
                }

                Text {
                    text: formatTime(minutes)
                }

                Text {
                    text: i18n("Change Time By:")
                }

                RowLayout {
                    TextField {
                        id: timeChangeField

                        Layout.preferredWidth: 100
                        inputMethodHints: Qt.ImhDigitsOnly

                        readonly property IntValidator intValidator: IntValidator {}

                        onTextChanged: {
                            var parsed = parseInt(timeChangeField.text);
                            dialog.changeMinutes = isNaN(parsed) ? 0 : parsed;
                            if (dialog.changeMinutes <= intValidator.bottom || dialog.changeMinutes >= intValidator.top) {
                                dialog.changeMinutes = 0;
                            }
                        }
                    }

                    Text {
                        text: i18nc("label after text field for minutes", "min")
                    }
                }

                Text {
                    text: i18n("Time After Change:")
                }

                Text {
                    text: formatTime(minutes + changeMinutes)
                }
            }

            Item {
                Layout.fillHeight: true
            }

            RowLayout {
                Button {
                    id: buttonBoxHistory
                    text: i18n("Edit History...")
                    icon.name: "document-edit"

                    hoverEnabled: true
                    ToolTip {
                        text: i18n("To change this task's time, you have to edit its event history")
                        visible: parent.hovered
                        delay: 500
                    }

                    onClicked: {
                        dialog.hide();
                        dialog.editHistory();
                    }
                }

                Item {
                    Layout.fillWidth: true
                }

                DialogButtonBox {
                    id: buttonBox
                    Layout.alignment: Qt.AlignRight

                    Button {
                        id: buttonBoxOk
                        text: i18n("OK")
                        icon.name: "dialog-ok"
                        DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
                        enabled: dialog.changeMinutes != 0
                    }

                    Button {
                        id: buttonBoxCancel
                        text: i18n("Cancel")
                        icon.name: "dialog-cancel"
                        DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
                    }

                    onAccepted: {
                        dialog.changeTime(dialog.taskUid, dialog.changeMinutes);
                        dialog.hide();
                    }
                    onRejected: dialog.hide()
                }
            }

            Keys.onEscapePressed: buttonBoxCancel.clicked()
            Keys.onReturnPressed: buttonBoxOk.clicked()
        }
    }

    function formatTime(minutes) {
        var abs = Math.abs(minutes);
        return "%1%2:%3"
            .arg(minutes < 0 ? '-' : '')
            .arg(Math.floor(abs / 60))
            .arg(abs % 60 >= 10 ? abs % 60 : "0" + abs % 60);
    }
}
