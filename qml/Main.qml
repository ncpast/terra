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
        onSosTriggered: console.log("SOS triggered at", sys.time)
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: statusBar.bottom
    }
}
