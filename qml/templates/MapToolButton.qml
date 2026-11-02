import QtQuick
import QtQuick.Layouts

Rectangle {
    id: btn

    property string label
    property string subcaption: ""
    property string value: ""
    property bool active: false
    property bool big: false
    signal clicked()

    implicitHeight: big ? 44 * 1.618 : 44
    color: active ? Theme.highlight : "transparent"

    Rectangle {
        width: parent.width
        height: 1
        color: Theme.fg3
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        spacing: 10

        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            spacing: big ? 8 : 2

            Text {
                Layout.fillWidth: true
                text: btn.label
                color: btn.active ? Theme.background_primary : Theme.highlight
                font.family: "Hack"
                font.weight: 700
                font.pixelSize: btn.big ? 14 : 12
                elide: Text.ElideRight
            }

            Text {
                Layout.fillWidth: true
                visible: btn.subcaption !== ""
                text: btn.subcaption
                color: btn.active ? Theme.background_primary : Theme.fg2
                opacity: btn.active ? 0.75 : 1
                font.family: "Hack"
                font.weight: 400
                font.pixelSize: btn.big ? 11 : 9
                elide: Text.ElideRight
            }
        }

        Text {
            Layout.alignment: Qt.AlignVCenter
            text: btn.value
            color: btn.active ? Theme.background_primary : Theme.highlight
            font.family: "Hack"
            font.weight: 700
            font.pixelSize: btn.big ? 14 : 12
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            btn.clicked()
            Qt.inputMethod.hide()
        }
    }
}