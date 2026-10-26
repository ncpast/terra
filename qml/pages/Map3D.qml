import QtQuick
import QtQuick.Layouts

Rectangle {
    anchors.fill: parent
    color: "transparent"

    Text {
        anchors.centerIn: parent
        font.family: "Hack"
        font.pixelSize: 24
        text: "3D Map\nWIP :-)"
        horizontalAlignment: Text.AlignHCenter
        color: Theme.highlight
    }
}
