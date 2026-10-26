import QtQuick
import QtQuick.Layouts

Rectangle {
    anchors.fill: parent
    color: "transparent"

    Text {
        anchors.centerIn: parent
        font.family: "Hack"
        font.pixelSize: 24
        text: "Notes\nWIP :-)"
        horizontalAlignment: Text.AlignHCenter
        color: Theme.highlight
    }
}
