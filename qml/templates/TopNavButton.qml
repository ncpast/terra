import QtQuick
import QtQuick.Controls.Basic

TabButton {
    id: root

    implicitHeight: 44

    contentItem: Text {
        font.family: "Hack"
        font.pixelSize: 13
        font.bold: checked
        font.letterSpacing: 0.8
        text: root.text
        color: root.checked ? Theme.background_primary : Theme.highlight
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        color: root.checked ? Theme.highlight : Theme.background_primary
        Rectangle {
            anchors.right: parent.right
            width: 1; height: parent.height
            color: Theme.highlight
        }
    }
}