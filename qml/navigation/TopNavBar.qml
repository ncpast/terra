import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import "../templates/"

Rectangle {
    id: root
    property alias currentTabIndex: tabs.currentIndex
    readonly property var names: ["MAP", "3D MAP", "ROUTES", "WEATHER", "ANNOTATIONS", "SYSTEM"]
    signal sosTriggered()

    implicitHeight: 45
    color: Theme.background_primary

    RowLayout {
        anchors.fill: parent
        anchors.bottomMargin: 1
        spacing: 0

        TabBar {
            id: tabs
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Repeater {
                model: root.names
                TopNavButton {
                    required property int index
                    required property string modelData
                    text: "[" + (index + 1) + "] " + modelData
                }
            }
        }

        AbstractButton {
            id: sos
            property real progress: 0
            Layout.preferredWidth: 112
            Layout.fillHeight: true

            topInset: 5
            bottomInset: 5
            leftInset: 5
            rightInset: 5

            onPressed: { progress = 0; tick.start() }
            onReleased: { tick.stop(); progress = 0 }
            onCanceled: { tick.stop(); progress = 0 }

            Timer {
                id: tick
                interval: 50
                repeat: true
                onTriggered: {
                    sos.progress += interval / 3000
                    if (sos.progress >= 1) {
                        stop()
                        sos.progress = 0
                        root.sosTriggered()
                    }
                }
            }

            background: Rectangle {
                id: background
                color: Theme.background_primary
                border.width: 2
                border.color: Theme.highlight
            }

            ColumnLayout {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.topMargin: 7
                spacing: -1

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "SOS"
                    color: Theme.highlight
                    font.family: "Hack"
                    font.pixelSize: 15
                    font.weight: 700
                    font.letterSpacing: 1.6
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "HOLD 3s"
                    color: "#9a9a9a"
                    font.family: "Hack"
                    font.pixelSize: 10
                }
            }

            Rectangle {
                x: sos.leftInset + background.border.width
                y: sos.topInset + background.border.width
                anchors.left: parent.left
                anchors.leftMargin: sos.leftInset + background.border.width
                anchors.bottom: parent.bottom
                anchors.bottomMargin: sos.bottomInset + background.border.width
                height: 11
                width: (parent.width - (sos.leftInset + sos.rightInset + 4)) * sos.progress
                color: Theme.highlight
            }
        }
    }
}