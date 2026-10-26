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
                { src: "pages/RoutesPage.qml", keep: false },
                { src: "pages/WeatherPage.qml", keep: false },
                { src: "pages/NotesPage.qml", keep: false },
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
