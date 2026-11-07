import QtQuick
import QtQuick.Layouts
import MapLibre 4.0
import Qt5Compat.GraphicalEffects

import "../templates/"
import "../"

ColumnLayout {
    id: root

    property real tabWidth: GlobalValues.toolBarWidth
    property int bottomInfoHeight: 24

    property bool followGps: true
    property bool relief: false
    property string layerMode: "map"
    property var gps: ({ lat: 49.2, lon: 20.0 })

    property var mapCoordinate: [49.2, 20.0]
    property real mapZoom: 15.4

    onMapZoomChanged: {
        if (mapZoom > 19) mapZoom = 19
        else if (mapZoom < 0) mapZoom = 0
    }

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

    // Waypoints

    property bool placing: false
    property int wpCounter: 0
    property int currentHighlightedWaypointId: -1

    function mercY(lat) {
        var s = Math.sin(lat * Math.PI / 180)
        return 0.5 - Math.log((1 + s) / (1 - s)) / (4 * Math.PI)
    }

    function placeWaypoint(px, py, w, h) {
        var world = 512 * Math.pow(2, mapZoom)
        var my = mercY(mapCoordinate[0]) * world + (py - h / 2)
        var lon = mapCoordinate[1] + (px - w / 2) / world * 360
        var lat = Math.atan(Math.sinh(Math.PI * (1 - 2 * my / world))) * 180 / Math.PI
        wpCounter++
        WaypointStore.add(lat, lon)
        placing = false
    }

    readonly property var highlightedWaypoint: {
        var list = WaypointStore.waypoints
        for (var i = 0; i < list.length; i++)
            if (list[i].id === currentHighlightedWaypointId)
                return list[i]
        return null
    }

    // Visible routes that contain the selected waypoint, and every waypoint of those routes:
    // drawn in the selection colour. The HUD still shows only the selected waypoint.
    readonly property var highlightedRouteIds: {
        var out = []
        if (currentHighlightedWaypointId === -1 || !showRoutes) return out
        var list = RouteStore.routes
        for (var i = 0; i < list.length; i++)
            if (list[i].shown !== false && list[i].waypointIds.indexOf(currentHighlightedWaypointId) !== -1)
                out.push(list[i].id)
        return out
    }

    readonly property var highlightedWaypointIds: {
        var out = [currentHighlightedWaypointId]
        var list = RouteStore.routes
        for (var i = 0; i < list.length; i++)
            if (highlightedRouteIds.indexOf(list[i].id) !== -1)
                for (var j = 0; j < list[i].waypointIds.length; j++)   // loop: C++ lists are not always real JS arrays
                    out.push(list[i].waypointIds[j])
        return out
    }

    // names of ALL routes the selected waypoint belongs to (hidden ones too: it is information
    // about the waypoint, not about what is drawn). Reads RouteStore.routes so renames update it.
    readonly property var selectedWaypointRouteNames: {
        var routes = RouteStore.routes   // binding dependency; routesUsing() alone would not refresh
        if (currentHighlightedWaypointId === -1 || routes.length === 0) return []
        var used = RouteStore.routesUsing(currentHighlightedWaypointId)
        var names = []
        for (var i = 0; i < used.length; i++) names.push(used[i].name)
        return names
    }

    // selected + part of any route: brighter alert; other highlighted waypoints: alert
    function waypointColor(waypointId) {
        if (waypointId === currentHighlightedWaypointId && selectedWaypointRouteNames.length > 0)
            return Theme.alertHighlight
        return isHighlighted(waypointId) ? Theme.alert : Theme.highlight
    }

    function isHighlighted(waypointId) {
        return waypointId !== -1 && highlightedWaypointIds.indexOf(waypointId) !== -1
    }

    // Submenus

    property int overlayDepth: 0    // 0 = tools, 1 = overlay menu, 2 = routes/waypoints list
    property string overlaySub: ""  // "routes" | "waypoints"
    property bool showRoutes: true
    property bool showWaypoints: true

    // Pathfinding

    readonly property var routeGeoJson: {
        var normal = [], highlighted = []
        var list = RouteStore.routes
        for (var i = 0; i < list.length; i++) {
            if (list[i].shown === false || !list[i].path || list[i].path.length < 2) continue
            var hl = highlightedRouteIds.indexOf(list[i].id) !== -1
            ;(hl ? highlighted : normal).push({
                type: "Feature", properties: { id: list[i].id, highlighted: hl },
                geometry: { type: "LineString", coordinates: list[i].path } })
        }
        // highlighted last, so they are drawn on top where routes overlap
        return { type: "FeatureCollection", features: showRoutes ? normal.concat(highlighted) : [] }
    }

    // line style: highlighted routes take the selection colour (data-driven, so one layer is enough)
    readonly property var routePaint: ({
        "line-color": ["case", ["get", "highlighted"], String(Theme.alert), String(Theme.highlight)],
        "line-width": ["case", ["get", "highlighted"], 4, 3]
    })

    property var routeSrc: null
    property var routeLyr: null

    Component {
        id: routeSrcComp
        SourceParameter {
            styleId: "routes"
            type: "geojson"
            property var data: ""
        }
    }

    Component {
        id: routeLyrComp
        LayerParameter {
            styleId: "routes-line"
            type: "line"
            property string source: "routes"
            layout: ({ "line-join": "round", "line-cap": "round" })
            paint: root.routePaint
        }
    }

    function pushRoutes() {
        if (routeLyr) { map.removeStyleParameter(routeLyr); routeLyr.destroy() }   // layer first, then its source
        if (routeSrc) { map.removeStyleParameter(routeSrc); routeSrc.destroy() }

        routeSrc = routeSrcComp.createObject(map, { "data": JSON.stringify(routeGeoJson) })
        routeLyr = routeLyrComp.createObject(map)
        map.addStyleParameter(routeSrc)
        map.addStyleParameter(routeLyr)
    }

    onRouteGeoJsonChanged: pushRoutes()
    Component.onCompleted: pushRoutes()          // create the first pair (empty) at startup

    Timer {
        id: dotTimer
        interval: 700; repeat: true; running: root.placing
        property int n: 1
        onTriggered: n = n % 3 + 1
        onRunningChanged: if (running) n = 1
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
                    label: "LAYERS"; value: root.layerMode.toUpperCase()
                    onClicked: root.layerMode = root.layerMode === "map" ? "sat"
                                              : root.layerMode === "sat" ? "topo" : "map"
                }

                MapToolButton {
                    Layout.fillWidth: true
                    label: "FOLLOW GPS"; value: root.followGps ? "[x]" : "[ ]"
                    active: root.followGps
                    onClicked: root.followGps = !root.followGps
                }

                MapToolButton {
                    Layout.fillWidth: true
                    label: "RELIEF"; value: root.relief ? "[x]" : "[ ]"
                    active: root.relief
                    onClicked: root.relief = !root.relief
                }

                MapToolButton {
                    Layout.fillWidth: true
                    label: "WAYPOINT"
                    value: root.placing ? ".".repeat(dotTimer.n) : "+"
                    active: root.placing
                    onClicked: root.placing = !root.placing
                }

                MapToolButton {
                    Layout.fillWidth: true
                    label: "OVERLAY"
                    value: ">"
                    onClicked: root.overlayDepth = 1
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Theme.fg3
                }

                Item { Layout.fillHeight: true }
            }

            // Overlays

            Item {
                id: overlayMenu
                anchors.fill: parent
                z: 5

                // page 1: overlay menu
                Rectangle {
                    width: parent.width; height: parent.height
                    x: root.overlayDepth >= 1 ? 0 : -width
                    z: 1
                    color: "black"
                    Behavior on x { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }

                    ColumnLayout {
                        anchors { left: parent.left; right: parent.right; top: parent.top }
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

                                text: "// OVERLAY"
                                color: Theme.fg2
                                font.family: "Hack"
                                font.pixelSize: 12
                            }
                        }

                        MapToolButton {
                            Layout.fillWidth: true
                            label: "< BACK"
                            onClicked: root.overlayDepth = 0
                        }

                        MapToolButton {
                            Layout.fillWidth: true
                            label: "ROUTES"
                            value: ">"
                            onClicked: { root.overlaySub = "routes"; root.overlayDepth = 2 }
                        }

                        MapToolButton {
                            Layout.fillWidth: true
                            label: "WAYPOINTS"
                            value: ">"
                            onClicked: { root.overlaySub = "waypoints"; root.overlayDepth = 2 }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: Theme.fg3
                        }
                    }
                }

                // page 2: visibility toggles for routes or waypoints
                Rectangle {
                    width: parent.width; height: parent.height
                    x: root.overlayDepth >= 2 ? 0 : -width
                    z: 2
                    color: "black"
                    Behavior on x { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }

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

                                text: "// " + root.overlaySub.toUpperCase()
                                color: Theme.fg2
                                font.family: "Hack"
                                font.pixelSize: 12
                            }
                        }

                        MapToolButton {
                            Layout.fillWidth: true
                            label: "< BACK"
                            onClicked: root.overlayDepth = 1
                        }
                        MapToolButton {
                            Layout.fillWidth: true
                            label: root.overlaySub === "routes" ? "ALL ROUTES" : "ALL WAYPOINTS"
                            value: (root.overlaySub === "routes" ? root.showRoutes : root.showWaypoints) ? "ON" : "OFF"
                            active: root.overlaySub === "routes" ? root.showRoutes : root.showWaypoints
                            onClicked: {
                                if (root.overlaySub === "routes") root.showRoutes = !root.showRoutes
                                else root.showWaypoints = !root.showWaypoints
                            }
                        }

                        ListView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            model: root.overlaySub === "waypoints" ? WaypointStore.waypoints : RouteStore.routes

                            delegate: MapToolButton {
                                readonly property bool isRoute: root.overlaySub === "routes"
                                readonly property bool shown: modelData.shown !== false
                                width: ListView.view.width
                                label: modelData.name
                                // a route without a calculated path has nothing to draw yet
                                value: isRoute && !(modelData.path && modelData.path.length > 1)
                                       ? "NO PATH" : (shown ? "ON" : "OFF")
                                active: shown
                                onClicked: {
                                    if (isRoute) RouteStore.setShown(modelData.id, !shown)
                                    else WaypointStore.setShown(index, !shown)
                                }
                            }

                            footer: Rectangle {
                                width: ListView.view.width
                                height: 1
                                color: Theme.fg3
                            }
                        }
                    }
                }
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
                id: map
                anchors.fill: parent
                style: "https://basemaps.cartocdn.com/gl/dark-matter-gl-style/style.json"
                zoomLevel: mapZoom
                coordinate: mapCoordinate

                Style {
                    id: mapStyle

                    SourceParameter {
                        id: routesSource
                        styleId: "routes"
                        type: "geojson"
                        property var data: '{"type":"FeatureCollection","features":[]}'
                    }

                    LayerParameter {
                        styleId: "routes-line"
                        type: "line"
                        property string source: "routes"
                        layout: ({ "line-join": "round", "line-cap": "round" })
                        paint: ({ "line-color": "#ffffff", "line-width": 3 })
                    }

                    SourceParameter {
                        styleId: "sat"
                        type: "raster"
                        property var tiles: ["https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}"]
                        property int tileSize: 256
                        property int maxzoom: 19
                    }

                    LayerParameter {
                        styleId: "sat-layer"
                        type: "raster"
                        property string source: "sat"
                        layout: ({ "visibility": root.layerMode === "sat" ? "visible" : "none" })
                    }

                    SourceParameter {
                        styleId: "topo"
                        type: "raster"
                        property var tiles: ["https://tile.opentopomap.org/{z}/{x}/{y}.png"]
                        property int tileSize: 256
                        property int maxzoom: 17
                    }

                    LayerParameter {
                        styleId: "topo-layer"
                        type: "raster"
                        property string source: "topo"
                        layout: ({ "visibility": root.layerMode === "topo" ? "visible" : "none" })
                    }

                    SourceParameter {
                        styleId: "relief"; type: "raster"
                        property var tiles: ["https://server.arcgisonline.com/ArcGIS/rest/services/Elevation/World_Hillshade/MapServer/tile/{z}/{y}/{x}"]
                        property int tileSize: 256
                    }

                    LayerParameter {
                        styleId: "relief-layer"; type: "raster"
                        property string source: "relief"
                        layout: ({ "visibility": root.relief ? "visible" : "none" })
                        paint: ({ "raster-opacity": 0.55 })
                    }
                }
            }

            // Waypoints

            Repeater {
                model: WaypointStore.waypoints
                delegate: Item {
                    z: 9
                    readonly property real world: 512 * Math.pow(2, root.mapZoom)

                    visible: root.showWaypoints && modelData.shown
                    x: parent.width / 2 + (modelData.lon - root.mapCoordinate[1]) / 360 * world
                    y: parent.height / 2 + (root.mercY(modelData.lat) - root.mercY(root.mapCoordinate[0])) * world

                    Rectangle {
                        width: 14; height: 14
                        x: -7; y: -7
                        rotation: 45
                        color: Theme.background_primary
                        border.color: root.waypointColor(modelData.id)
                        border.width: 2
                    }
                    Text {
                        x: 14; y: -8
                        text: modelData.name
                        color: root.waypointColor(modelData.id)
                        font { family: "monospace"; pixelSize: 12; bold: true }
                        visible: mapZoom < 11 ? false : true
                    }

                    MouseArea {
                        id: wpMArea
                        x: -12; y: -12
                        width: 24; height: 24
                        enabled: !root.placing
                        onClicked: root.currentHighlightedWaypointId = modelData.id
                    }
                }
            }

            MapHud {
                anchors.fill: parent
                lat: root.highlightedWaypoint ? root.highlightedWaypoint.lat : root.gps.lat
                lon: root.highlightedWaypoint ? root.highlightedWaypoint.lon : root.gps.lon
                alert: root.currentHighlightedWaypointId !== -1
                routeNames: root.selectedWaypointRouteNames
                zoom: root.mapZoom
                heading: 0
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
                    property real pressX: 0
                    property real pressY: 0
                    property bool moved: false

                    onPressed: (m) => { px = m.x; py = m.y; pressX = m.x; pressY = m.y; moved = false }

                    onPositionChanged: (m) => {
                        if (!moved) {
                            if (Math.abs(m.x - pressX) <= 8 && Math.abs(m.y - pressY) <= 8)
                                return
                            moved = true
                            root.placing = false
                            px = m.x; py = m.y
                            return
                        }
                        root.followGps = false
                        var dx = m.x - px, dy = m.y - py
                        px = m.x; py = m.y
                        var degPerPx = 360 / (512 * Math.pow(2, mapZoom))
                        var lat = mapCoordinate[0], lon = mapCoordinate[1]
                        mapCoordinate = [lat + dy * degPerPx * Math.cos(lat * Math.PI / 180),
                                         lon - dx * degPerPx]
                    }

                    onClicked: (m) => {
                        if (root.placing && !moved) root.placeWaypoint(m.x, m.y, width, height)
                        if (!moved) root.currentHighlightedWaypointId = -1
                    }

                    onWheel: (w) => {
                        root.placing = false
                        mapZoom += w.angleDelta.y / 600
                    }
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