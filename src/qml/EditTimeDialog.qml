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

            Kirigami.FormLayout {
                Layout.alignment: Qt.AlignHCenter

                Text {
                    Kirigami.FormData.label: i18n("Task Name:")
                    text: taskName

                    font.bold: true
                    wrapMode: Text.Wrap
                }

                Text {
                    Kirigami.FormData.label: i18n("Task Description:")
                    text: taskDescription

                    wrapMode: Text.Wrap
                    elide: Text.ElideRight
                    maximumLineCount: 5
                }

                Kirigami.Separator {
                    Kirigami.FormData.isSection: true
                }

                Text {
                    Kirigami.FormData.label: i18n("Current Time:")
                    text: formatTime(minutes)
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

                    Kirigami.FormData.label: i18n("Change Time By:")
                }

                Text {
                    Kirigami.FormData.label: i18n("Time After Change:")
                    text: formatTime(minutes + changeMinutes)
                }
            }

            Item {
                Layout.fillHeight: true
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
