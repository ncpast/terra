import QtQuick
import QtQuick.Layouts
import "../templates"

Rectangle {
    id: root

    property real tabWidth: GlobalValues.toolBarWidth
    property string selectedTab: "waypoints"

    property int selectedWaypointId: -1
    property int selectedRouteId: -1

    Connections {
        target: WaypointStore
        function onWaypointRemoved(id) { RouteStore.removeWaypoint(id) }
    }

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

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Theme.fg3
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
                            selectedRouteId = RouteStore.add()
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

            ColumnLayout {
                anchors.fill: parent
                visible: selectedTab === "routes"
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
                        text: "// ROUTES"
                        color: Theme.fg2
                        font.family: "Hack"; font.pixelSize: 12
                    }
                    Text {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignRight
                        text: "NAME, CHECKPOINTS"
                        color: Theme.fg2
                        font.family: "Hack"; font.pixelSize: 12
                    }
                }

                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: RouteStore.routes

                    delegate: MapToolButton {
                        width: ListView.view.width
                        big: true
                        label: modelData.name
                        subcaption: modelData.waypointIds.length +
                                    (modelData.waypointIds.length === 1 ? " CHECKPOINT" : " CHECKPOINTS")
                        active: modelData.id === selectedRouteId
                        onClicked: selectedRouteId = modelData.id
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

        // Waypoint editor

        Item {
            id: waypointEditor

            visible: selectedTab === "waypoints"
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

            readonly property var usedIn: {
                var _dep = RouteStore.routes
                return wp ? RouteStore.routesUsing(wp.id) : []
            }

            function removeFromRoute(routeId, waypointId) {
                var list = RouteStore.routes
                for (var i = 0; i < list.length; i++) {
                    if (list[i].id !== routeId) continue
                    var kept = list[i].waypointIds.filter(function (x) { return x !== waypointId })
                    RouteStore.setCheckpoints(routeId, kept)
                    return
                }
            }

            // Confirmation expires after 3 seconds

            Timer {
                id: confirmTimer
                interval: 3000
                onTriggered: waypointEditor.confirmingDelete = false
            }

            // When nothing is selected

            Text {
                anchors.centerIn: parent
                visible: !waypointEditor.wp
                text: "// SELECT A WAYPOINT"
                color: Theme.fg2
                font.family: "Hack"; font.pixelSize: 12
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 16
                visible: waypointEditor.wp !== null

                // Top row

                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        Layout.fillWidth: true
                        text: waypointEditor.wp ? waypointEditor.wp.name : ""
                        color: Theme.highlight
                        font.family: "Hack"; font.pixelSize: 24; font.weight: 700
                        elide: Text.ElideRight
                    }
                    Text {
                        text: waypointEditor.wp ? "ID " + waypointEditor.wp.id : ""
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
                        filled: waypointEditor.wpVisible
                        onClicked: waypointEditor.wpVisible = !waypointEditor.wpVisible
                    }
                }

                Text {
                    text: waypointEditor.error ? "! " + waypointEditor.error : ""
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
                        onClicked: waypointEditor.save()
                    }
                    ActionButton {
                        Layout.fillWidth: true
                        label: waypointEditor.confirmingDelete ? "TAP TO CONFIRM" : "DELETE"
                        onClicked: {
                            if (!waypointEditor.confirmingDelete) {
                                if (waypointEditor.usedIn.length > 0)
                                    waypointEditor.error = "WILL BE REMOVED FROM "
                                            + waypointEditor.usedIn.length + " ROUTE(S). TAP AGAIN TO CONFIRM"
                                waypointEditor.confirmingDelete = true
                                confirmTimer.restart()
                            } else {
                                WaypointStore.remove(waypointEditor.sid)
                                root.selectedWaypointId = -1
                            }
                        }
                    }
                }

                Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.fg3 }

                Text {
                    text: "// USED IN ROUTES (" + waypointEditor.usedIn.length + ")"
                    color: Theme.fg2
                    font.family: "Hack"; font.pixelSize: 12
                }

                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: waypointEditor.usedIn

                    delegate: Item {
                        width: ListView.view.width
                        height: 48

                        RowLayout {
                            anchors.fill: parent
                            spacing: 8

                            Text {
                                Layout.fillWidth: true
                                text: modelData.name
                                color: Theme.highlight
                                elide: Text.ElideRight
                                font.family: "Hack"; font.pixelSize: 14; font.weight: 700
                            }
                            ActionButton {
                                Layout.preferredWidth: 64; Layout.preferredHeight: 36
                                label: "VIEW"
                                onClicked: {
                                    root.selectedRouteId = modelData.id
                                    root.selectedTab = "routes"
                                }
                            }
                            ActionButton {
                                Layout.preferredWidth: 44; Layout.preferredHeight: 36
                                label: "x"
                                onClicked: waypointEditor.removeFromRoute(modelData.id, waypointEditor.sid)
                            }
                        }

                        Rectangle {
                            anchors.bottom: parent.bottom
                            width: parent.width; height: 1
                            color: Theme.fg3
                        }
                    }

                    Text {
                        anchors.centerIn: parent
                        visible: parent.count === 0
                        text: "NOT USED IN ANY ROUTE"
                        color: Theme.fg2
                        font.family: "Hack"; font.pixelSize: 12
                    }
                }
            }
        }

        // Route editor

        Item {
            id: routeEditor

            visible: selectedTab === "routes"
            Layout.fillWidth: true
            Layout.fillHeight: true

            property int sid: root.selectedRouteId
            property bool confirmingDelete: false
            property string error: ""

            // Finds selected route based on ID

            function find(id) {
                var list = RouteStore.routes
                for (var i = 0; i < list.length; i++)
                    if (list[i].id === id)
                        return list[i]
                return null
            }

            readonly property var route: find(root.selectedRouteId)

            function load() {
                picking = false
                confirmingDelete = false
                error = ""
                var r = find(sid)
                if (!r) return
                routeNameField.text = r.name
            }
            onSidChanged: load()

            function save() {
                if (routeNameField.text.trim() === "") { error = "NAME CANNOT BE EMPTY"; return }
                error = ""
                RouteStore.rename(sid, routeNameField.text.trim())
            }

            property bool picking: false

            function wpName(id) {   // reads WaypointStore.waypoints, so it stays up to date
                var list = WaypointStore.waypoints
                for (var i = 0; i < list.length; i++)
                    if (list[i].id === id)
                        return list[i].name
                return "(DELETED)"
            }

            function setList(list) { RouteStore.setCheckpoints(sid, list) }

            function move(i, delta) {
                var list = route.waypointIds.slice()
                var j = i + delta
                if (j < 0 || j >= list.length) return
                var tmp = list[i]; list[i] = list[j]; list[j] = tmp
                setList(list)
            }

            function removeAt(i) {
                var list = route.waypointIds.slice()
                list.splice(i, 1)
                setList(list)
            }

            function addCheckpoint(waypointId) {
                var list = route.waypointIds.slice()
                list.push(waypointId)   // the same waypoint can be added again
                setList(list)
                picking = false
            }

            // Confirmation expires after 3 seconds

            Timer {
                id: routeConfirmTimer
                interval: 3000
                onTriggered: routeEditor.confirmingDelete = false
            }

            // When nothing is selected

            Text {
                anchors.centerIn: parent
                visible: !routeEditor.route
                text: "// SELECT A ROUTE"
                color: Theme.fg2
                font.family: "Hack"; font.pixelSize: 12
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 16
                visible: routeEditor.route !== null

                // Top row

                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        Layout.fillWidth: true
                        text: routeEditor.route ? routeEditor.route.name : ""
                        color: Theme.highlight
                        font.family: "Hack"; font.pixelSize: 24; font.weight: 700
                        elide: Text.ElideRight
                    }
                    Text {
                        text: routeEditor.route ? "ID " + routeEditor.route.id : ""
                        color: Theme.fg2
                        font.family: "Hack"; font.pixelSize: 12
                    }
                }

                Field { id: routeNameField; Layout.fillWidth: true; caption: "NAME" }

                Text {
                    text: routeEditor.error ? "! " + routeEditor.error : ""
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
                        onClicked: routeEditor.save()
                    }
                    ActionButton {
                        Layout.fillWidth: true
                        label: routeEditor.confirmingDelete ? "TAP TO CONFIRM" : "DELETE"
                        onClicked: {
                            if (waypointEditor.usedIn.length > 0) {
                                waypointEditor.error = "USED IN " + waypointEditor.usedIn.length + " ROUTE(S): REMOVE IT FROM THEM FIRST"
                                return
                            }
                            if (!routeEditor.confirmingDelete) {
                                routeEditor.confirmingDelete = true
                                routeConfirmTimer.restart()
                            } else {
                                RouteStore.remove(routeEditor.sid)
                                root.selectedRouteId = -1
                            }
                        }
                    }
                }

                Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.fg3 }

                Text {
                    text: routeEditor.picking ? "// PICK A WAYPOINT" : "// CHECKPOINTS"
                    color: Theme.fg2
                    font.family: "Hack"; font.pixelSize: 12
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    // ordered checkpoints
                    ListView {
                        anchors.fill: parent
                        visible: !routeEditor.picking
                        clip: true
                        model: routeEditor.route ? routeEditor.route.waypointIds : []

                        delegate: Item {
                            width: ListView.view.width
                            height: 48

                            RowLayout {
                                anchors.fill: parent
                                spacing: 8

                                Text {
                                    Layout.preferredWidth: 24
                                    text: index + 1
                                    color: Theme.fg2
                                    font.family: "Hack"; font.pixelSize: 13
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: routeEditor.wpName(modelData)
                                    color: Theme.highlight
                                    elide: Text.ElideRight
                                    font.family: "Hack"; font.pixelSize: 14; font.weight: 700
                                }
                                ActionButton {
                                    Layout.preferredWidth: 44; Layout.preferredHeight: 36
                                    label: "^"
                                    onClicked: routeEditor.move(index, -1)
                                }
                                ActionButton {
                                    Layout.preferredWidth: 44; Layout.preferredHeight: 36
                                    label: "v"
                                    onClicked: routeEditor.move(index, 1)
                                }
                                ActionButton {
                                    Layout.preferredWidth: 44; Layout.preferredHeight: 36
                                    label: "x"
                                    onClicked: routeEditor.removeAt(index)
                                }
                            }

                            Rectangle {
                                anchors.bottom: parent.bottom
                                width: parent.width; height: 1
                                color: Theme.fg3
                            }
                        }

                        Text {
                            anchors.centerIn: parent
                            visible: parent.count === 0
                            text: "NO CHECKPOINTS YET"
                            color: Theme.fg2
                            font.family: "Hack"; font.pixelSize: 12
                        }
                    }

                    // waypoint picker
                    ListView {
                        anchors.fill: parent
                        visible: routeEditor.picking
                        clip: true
                        model: WaypointStore.waypoints

                        delegate: MapToolButton {
                            width: ListView.view.width
                            label: modelData.name
                            value: modelData.lat.toFixed(3) + ", " + modelData.lon.toFixed(3)
                            onClicked: routeEditor.addCheckpoint(modelData.id)
                        }

                        Text {
                            anchors.centerIn: parent
                            visible: parent.count === 0
                            text: "NO WAYPOINTS YET"
                            color: Theme.fg2
                            font.family: "Hack"; font.pixelSize: 12
                        }
                    }
                }

                ActionButton {
                    Layout.fillWidth: true
                    label: routeEditor.picking ? "CANCEL" : "+ ADD CHECKPOINT"
                    filled: !routeEditor.picking
                    onClicked: routeEditor.picking = !routeEditor.picking
                }
            }
        }
    }
}
