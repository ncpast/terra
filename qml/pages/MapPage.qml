import QtQuick
import QtQuick.Layouts
import MapLibre 4.0

import "../templates/"

ColumnLayout {
    id: root
    property real tabWidth: GlobalValues.toolBarWidth
    property int bottomInfoHeight: 24

    property bool followGps: true
    property bool satellite: false
    property var gps: ({ lat: 49.2, lon: 20.0 })

    property var mapCoordinate: [49.2, 20.0]
    property real mapZoom: 12

    onFollowGpsChanged: if (followGps) mapCoordinate = [gps.lat, gps.lon]

    // GPS going around in circles to test follow GPS

    property real gpsAngle: 0
    property var gpsCenter: ({ lat: 49.2, lon: 20.0 })

    Timer {
        interval: 1000
        running: true
        repeat: true

        onTriggered: {
            root.gpsAngle += 0.15

            var radius = 0.001
            var lat = root.gpsCenter.lat
                      + radius * Math.sin(root.gpsAngle)

            var lon = root.gpsCenter.lon
                      + radius / Math.cos(root.gpsCenter.lat * Math.PI / 180)
                      * Math.cos(root.gpsAngle)

            root.gps = { lat: lat, lon: lon }

            if (root.followGps)
                root.mapCoordinate = [lat, lon]
        }
    }

    spacing: 0

    RowLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        spacing: 0

        Rectangle {
            id: panel
            Layout.preferredWidth: tabWidth - 1
            Layout.fillHeight: true
            color: "transparent"

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

                        text: "// MAP TOOLS"
                        color: Theme.fg2
                        font.family: "Hack"
                        font.pixelSize: 12
                    }
                }

                MapToolButton {
                    Layout.fillWidth: true
                    label: "FOLLOW GPS"; value: root.followGps ? "[x]" : "[ ]"
                    active: root.followGps
                    onClicked: root.followGps = !root.followGps
                }

                MapToolButton {
                    Layout.fillWidth: true
                    label: "LAYERS"; value: root.satellite ? "SAT" : "MAP"
                    onClicked: root.satellite = !root.satellite
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Theme.fg3
                }

                Item { Layout.fillHeight: true }
            }
        }

        Rectangle {
            Layout.preferredWidth: 1
            Layout.fillHeight: true
            color: Theme.highlight
        }

        Item {   // map area
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            MapLibre {
                visible: !satellite
                id: map
                anchors.fill: parent
                style: "https://basemaps.cartocdn.com/gl/dark-matter-gl-style/style.json"
                zoomLevel: mapZoom
                coordinate: mapCoordinate
            }

            MapLibre {
                visible: satellite
                id: satelliteMap
                anchors.fill: parent
                style: "file:///home/siro/Desktop/Projects/TERRA/terra/styles/satellite.json"
                zoomLevel: mapZoom
                coordinate: mapCoordinate
            }

            Item {
                id: marker
                width: 16; height: 16
                readonly property real world: 512 * Math.pow(2, mapZoom)
                function mx(lon) { return (lon + 180) / 360 }
                function my(lat) {
                    var s = Math.sin(lat * Math.PI / 180)
                    return 0.5 - Math.log((1 + s) / (1 - s)) / (4 * Math.PI)
                }
                x: map.width  / 2 + (mx(root.gps.lon) - mx(mapCoordinate[1])) * world - width / 2
                y: map.height / 2 + (my(root.gps.lat) - my(mapCoordinate[0])) * world - height / 2

                Rectangle { anchors.fill: parent; radius: 8; color: "transparent"
                            border.width: 2; border.color: Theme.highlight }
                Rectangle { anchors.centerIn: parent; width: 6; height: 6; radius: 3; color: Theme.highlight }
            }

            Column {
                z: 10
                anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8
                spacing: 4
                Repeater {
                    model: [ { t: "+", d: 1 }, { t: "−", d: -1 } ]
                    Rectangle {
                        required property var modelData
                        width: 44; height: 44
                        color: Theme.background_primary
                        border.width: 1; border.color: Theme.highlight
                        Text { anchors.centerIn: parent; text: modelData.t
                               color: Theme.highlight; font.family: "Hack"; font.pixelSize: 22 }
                        MouseArea { anchors.fill: parent; onClicked: mapZoom += modelData.d }
                    }
                }
            }

            PinchArea {
                anchors.fill: parent
                property real startZoom: 0
                onPinchStarted: startZoom = mapZoom
                onPinchUpdated: (p) => mapZoom = startZoom + Math.log(p.scale) / Math.LN2

                MouseArea {
                    anchors.fill: parent
                    property real px: 0
                    property real py: 0

                    onPressed: (m) => { px = m.x; py = m.y }
                    onPositionChanged: (m) => {
                        root.followGps = false
                        var dx = m.x - px, dy = m.y - py
                        px = m.x; py = m.y
                        var degPerPx = 360 / (512 * Math.pow(2, mapZoom))
                        var lat = mapCoordinate[0], lon = mapCoordinate[1]
                        mapCoordinate = [lat + dy * degPerPx * Math.cos(lat * Math.PI / 180),
                                          lon - dx * degPerPx]
                    }
                    onWheel: (w) => mapZoom += w.angleDelta.y / 600
                }
            }
        }
    }

    Rectangle {
        Layout.preferredHeight: 1
        Layout.fillWidth: true
        color: Theme.highlight
    }

    Rectangle {
        Layout.preferredHeight: bottomInfoHeight
        Layout.fillWidth: true
        color: "transparent"

        Text {
            color: Theme.highlight
            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            font.pixelSize: 12
            text: "> ROUTE X→Y 0000 | 0km | +0m | ETA 00:00 | MAX SLOPE 0°"
            font.family: "Hack"
        }

        Rectangle {
            color: Theme.highlight
            anchors.right: parent.right
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            width: 7
            height: 16

            Timer {
                interval: 750
                running: true
                repeat: true
                onTriggered: {
                    parent.opacity = parent.opacity === 1.0 ? 0.0 : 1.0
                }
            }
        }
    }
}