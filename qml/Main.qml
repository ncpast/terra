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

    StatusBar {
        id: statusBar
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: statusBar.bottom
        height: 1
        color: Theme.highlight
    }
}
