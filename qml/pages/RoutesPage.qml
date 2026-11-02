import QtQuick
import QtQuick.Layouts
import "../templates"

Rectangle {
    id: root

    property real tabWidth: GlobalValues.toolBarWidth
    property string selectedTab: "waypoints"

    property int selectedWaypointId: -1
    property int selectedRouteId: -1

    component Field: ColumnLayout {
        id: field
        property string caption
        property alias text: input.text
        property int hints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
        spacing: 6

        Text {
            text: field.caption
            color: Theme.fg2
            font.family: "Hack"; font.pixelSize: 11
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            color: "transparent"
            border.color: Theme.highlight
            border.width: 1

            TextInput {
                id: input
                inputMethodHints: field.hints
                anchors.fill: parent
                anchors.margins: 10
                verticalAlignment: TextInput.AlignVCenter
                color: Theme.highlight
                font.family: "Hack"; font.pixelSize: 14
                selectByMouse: true
                clip: true
            }
        }
    }

    component ActionButton: Rectangle {
        id: ab
        property string label
        property bool filled: false
        signal clicked()

        implicitHeight: 44
        color: filled ? Theme.highlight : "transparent"
        border.color: Theme.highlight
        border.width: 1

        Text {
            anchors.centerIn: parent
            text: ab.label
            color: ab.filled ? Theme.background_primary : Theme.highlight
            font.family: "Hack"; font.pixelSize: 13; font.weight: 700
        }
        MouseArea { anchors.fill: parent; onClicked: ab.clicked() }
    }

    anchors.fill: parent
    color: "transparent"

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Item {
            Layout.preferredWidth: tabWidth - 1
            Layout.fillHeight: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 24
                    color: "transparent"

                    Text {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        verticalAlignment: Text.AlignVCenter
                        text: "// TYPE"
                        color: Theme.fg2
                        font.family: "Hack"
                        font.pixelSize: 12
                    }
                }

                MapToolButton {
                    Layout.fillWidth: true
                    label: "WAYPOINTS"
                    active: selectedTab === "waypoints"
                    onClicked: selectedTab = "waypoints"
                }

                MapToolButton {
                    Layout.fillWidth: true
                    label: "ROUTES"
                    active: selectedTab === "routes"
                    onClicked: selectedTab = "routes"
                }

                Item { Layout.fillHeight: true }

                MapToolButton {
                    Layout.fillWidth: true
                    label: "NEW +"
                    subcaption: selectedTab === "waypoints" ? "WAYPOINT" : "ROUTE"
                    onClicked: {
                        if (selectedTab === "waypoints")
                            root.selectedWaypointId = WaypointStore.add(0, 0)
                        else
                            console.log("TODO: create route")
                    }
                }

                Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.fg3 }
            }
        }

        Rectangle {
            Layout.preferredWidth: 1
            Layout.fillHeight: true
            color: Theme.highlight
        }

        // List of selected

        Item {
            Layout.preferredWidth: tabWidth * 2.5 - 1
            Layout.fillHeight: true

            // Waypoints

            ColumnLayout {
                anchors.fill: parent
                visible: selectedTab === "waypoints"
                spacing: 0

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 24
                    color: "transparent"

                    Text {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        verticalAlignment: Text.AlignVCenter
                        text: "// WAYPOINTS"
                        color: Theme.fg2
                        font.family: "Hack"
                        font.pixelSize: 12
                    }

                    Text {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignRight
                        text: "NAME, LAT / LON"
                        color: Theme.fg2
                        font.family: "Hack"
                        font.pixelSize: 12
                    }
                }

                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: WaypointStore.waypoints

                    delegate: MapToolButton {
                        width: ListView.view.width
                        big: true
                        label: modelData.name
                        subcaption: modelData.lat.toFixed(5) + " N  " + modelData.lon.toFixed(5) + " E"
                        value: modelData.shown ? "SHOWN" : "HIDDEN"
                        active: modelData.id === selectedWaypointId
                        onClicked: selectedWaypointId = modelData.id
                    }

                    footer: Rectangle {
                        width: ListView.view.width
                        height: 1
                        color: Theme.fg3
                    }
                }
            }
        }

        Rectangle {
            Layout.preferredWidth: 1
            Layout.fillHeight: true
            color: Theme.highlight
        }

        // Editing tab
        Item {
            id: editor
            Layout.fillWidth: true
            Layout.fillHeight: true

            property int sid: root.selectedWaypointId
            property bool confirmingDelete: false
            property bool wpVisible: true
            property string error: ""

            onErrorChanged: console.log(error)

            // Finds selected waypoint based on ID

            function find(id) {
                var list = WaypointStore.waypoints
                for (var i = 0; i < list.length; i++)
                    if (list[i].id === id)
                        return list[i]
                return null
            }

            readonly property var wp: find(root.selectedWaypointId)

            function load() {
                confirmingDelete = false
                error = ""
                var w = find(sid)
                if (!w) return
                nameField.text = w.name
                latField.text = w.lat.toFixed(5)
                lonField.text = w.lon.toFixed(5)
                wpVisible = w.shown
            }
            onSidChanged: load()

            function save() {
                var lat = parseFloat(latField.text)
                var lon = parseFloat(lonField.text)
                if (nameField.text.trim() === "") { error = "NAME CANNOT BE EMPTY"; return }
                if (isNaN(lat) || lat < -90 || lat > 90) { error = "LATITUDE MUST BE -90..90"; return }
                if (isNaN(lon) || lon < -180 || lon > 180) { error = "LONGITUDE MUST BE -180..180"; return }
                error = ""
                WaypointStore.update(sid, nameField.text.trim(), lat, lon, wpVisible)
            }

            // Confirmation expires after 3 seconds

            Timer {
                id: confirmTimer
                interval: 3000
                onTriggered: editor.confirmingDelete = false
            }

            // When nothing is selected

            Text {
                anchors.centerIn: parent
                visible: !editor.wp
                text: "// SELECT A WAYPOINT"
                color: Theme.fg2
                font.family: "Hack"; font.pixelSize: 12
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 16
                visible: editor.wp !== null

                // Top row

                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        Layout.fillWidth: true
                        text: editor.wp ? editor.wp.name : ""
                        color: Theme.highlight
                        font.family: "Hack"; font.pixelSize: 24; font.weight: 700
                        elide: Text.ElideRight
                    }
                    Text {
                        text: editor.wp ? "ID " + editor.wp.id : ""
                        color: Theme.fg2
                        font.family: "Hack"; font.pixelSize: 12
                    }
                }

                Field { id: nameField; Layout.fillWidth: true; caption: "NAME" }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 18

                    Field {
                        id: latField
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        Layout.preferredWidth: 1
                        caption: "LATITUDE"
                        hints: Qt.ImhFormattedNumbersOnly
                    }

                    Field {
                        id: lonField
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        Layout.preferredWidth: 1
                        caption: "LONGITUDE"
                        hints: Qt.ImhFormattedNumbersOnly
                    }

                    ActionButton {
                        Layout.alignment: Qt.AlignBottom
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        Layout.preferredWidth: 1
                        label: "VISIBILITY"
                        filled: editor.wpVisible
                        onClicked: editor.wpVisible = !editor.wpVisible
                    }
                }

                Text {
                    text: editor.error ? "! " + editor.error : ""
                    color: Theme.alert
                    font.family: "Hack"; font.pixelSize: 12
                }

                // Bottom buttons

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    ActionButton {
                        Layout.fillWidth: true
                        label: "SAVE CHANGES"
                        filled: true
                        onClicked: editor.save()
                    }
                    ActionButton {
                        Layout.fillWidth: true
                        label: editor.confirmingDelete ? "TAP TO CONFIRM" : "DELETE"
                        onClicked: {
                            if (!editor.confirmingDelete) {
                                editor.confirmingDelete = true
                                confirmTimer.restart()
                            } else {
                                WaypointStore.remove(editor.sid)
                                root.selectedWaypointId = -1
                            }
                        }
                    }
                }

                Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.fg3 }

                Text {
                    text: "// USED IN ROUTES"
                    color: Theme.fg2
                    font.family: "Hack"; font.pixelSize: 12
                }

                Text {
                    text: "NO ROUTES YET"
                    color: Theme.fg2
                    font.family: "Hack"; font.pixelSize: 12
                }

                Item { Layout.fillHeight: true }
            }
        }
    }
}
