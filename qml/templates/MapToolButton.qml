import QtQuick

Rectangle {
    id: btn

    property string label
    property string value: ""
    property bool active: false
    signal clicked()

    implicitHeight: 44
    color: active ? Theme.highlight : "transparent"

    Rectangle {
        width: parent.width
        height: 1
        color: Theme.fg3
    }

    Text {
        anchors.left: parent.left; anchors.leftMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        text: btn.label
        color: btn.active ? Theme.background_primary : Theme.highlight
        font.family: "Hack"
        font.weight: 700
        font.pixelSize: 12
    }

    Text {
        anchors.right: parent.right; anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        text: btn.value
        color: btn.active ? Theme.background_primary : Theme.highlight
        font.family: "Hack"
        font.weight: 700
        font.pixelSize: 12
    }

    MouseArea {
        anchors.fill: parent
        onClicked: btn.clicked()
    }
}