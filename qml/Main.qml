import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import QtQuick.VirtualKeyboard

import "./navigation/"
import terra

ApplicationWindow {
    id: window
    width: 1024
    height: 600
    minimumWidth: 1024
    minimumHeight: 600
    maximumWidth: 1024
    maximumHeight: 600
    visible: true
    title: qsTr("TERRA")
    color: Theme.background_primary

    InputPanel {
        id: keyboard
        z: 100
        width: parent.width
        y: Qt.inputMethod.visible ? parent.height - height : parent.height
        visible: Qt.inputMethod.visible
    }

    Rectangle {
        id: hideKeyboardButton
        z: 101
        visible: Qt.inputMethod.visible
        width: 96
        height: 44
        //x: (parent.width - width) / 2
        y: keyboard.y - height
        color: Theme.background_primary
        border.color: Theme.highlight
        border.width: 1

        Text {
            anchors.centerIn: parent
            text: "[v]"
            color: Theme.highlight
            font.family: "Hack"
            font.pixelSize: 16
            font.weight: 700
        }

        MouseArea {
            anchors.fill: parent
            onClicked: Qt.inputMethod.hide()
        }
    }

    SystemStatus { id: sys }

    StatusBar {
        id: statusBar
    }

    TopNavBar {
        id: topBar
        onSosTriggered: console.log("SOS triggered at", sys.time)
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: statusBar.bottom
    }

    StackLayout {
        anchors.top: topBar.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        currentIndex: topBar.currentTabIndex

        Repeater {
            model: [
                { src: "pages/MapPage.qml", keep: true  },
                { src: "pages/Map3D.qml", keep: false },
                { src: "pages/NotesPage.qml", keep: false },
                { src: "pages/RoutesPage.qml", keep: false },
                { src: "pages/WeatherPage.qml", keep: false },
                { src: "pages/SystemPage.qml", keep: false },
            ]

            Loader {
                id: pageLoader
                required property var modelData
                required property int index
                source: modelData.src
                active: StackLayout.isCurrentItem || modelData.keep
            }
        }
    }
}
