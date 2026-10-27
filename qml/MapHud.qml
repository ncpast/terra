import QtQuick

import QtQuick.Shapes

Item {
    id: hud
    property real lat: 0
    property real lon: 0
    property real zoom: 0
    property real heading: 0          // degrees, 0 = north; later from IMU/GPS

    readonly property color fg: "white"
    readonly property color bg: "black"

    function fmt(v, pos, neg) {
        return Math.abs(v).toFixed(5) + "° " + (v >= 0 ? pos : neg)
    }

    // compass
    Rectangle {
        id: compass
        anchors {
            left: parent.left
            top: parent.top
            margins: 8
        }

        width: 64
        height: 64
        radius: width / 2

        color: hud.bg
        border.color: hud.fg
        border.width: 1
        clip: true

        Item {
            id: upperHalf
            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
            }
            height: parent.height / 2
            clip: true

            Shape {
                anchors.fill: parent
                rotation: hud.heading

                ShapePath {
                    fillColor: hud.fg
                    strokeColor: hud.fg
                    strokeWidth: .1

                    startX: upperHalf.width / 2
                    startY: 12

                    PathLine {
                        x: upperHalf.width / 2 + 6
                        y: 25
                    }
                    PathLine {
                        x: upperHalf.width / 2 - 6
                        y: 25
                    }
                    PathLine {
                        x: upperHalf.width / 2
                        y: 12
                    }
                }
            }
        }

        Item {
            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
            }
            height: parent.height / 2

            Text {
                anchors.centerIn: parent
                anchors.verticalCenterOffset: -6

                text: "N"
                color: hud.fg

                font {
                    family: "Hack"
                    pixelSize: 16
                    bold: true
                }
            }
        }
    }

    // coordinate frame
    Rectangle {
        anchors { left: parent.left; bottom: parent.bottom; margins: 8 }
        width: coords.implicitWidth + 16; height: coords.implicitHeight + 10
        color: hud.bg; border.color: hud.fg; border.width: 1
        Column {
            id: coords
            anchors.centerIn: parent
            spacing: 2
            Text { text: "LAT " + hud.fmt(hud.lat, "N", "S"); color: hud.fg; font { family: "Hack"; pixelSize: 13 } }
            Text { text: "LON " + hud.fmt(hud.lon, "E", "W"); color: hud.fg; font { family: "Hack"; pixelSize: 13 } }
        }
    }

    // scale bar

    readonly property real pixelMm: 0.15      // physical pixel size of the 7" 1024x600 screen
    readonly property real mpp: 40075016.686 * Math.cos(lat * Math.PI / 180) / (512 * Math.pow(2, zoom))

    readonly property real niceDist: {
        var target = barMax * mpp
        var p = Math.pow(10, Math.floor(Math.log(target) / Math.LN10))
        var n = target / p
        return (n >= 5 ? 5 : n >= 2 ? 2 : 1) * p
    }
    readonly property real barPx: niceDist / mpp

    function distText(m) { return m >= 1000 ? (m / 1000) + "km" : m + "m" }
    function ratioText() {
        var n = mpp / (pixelMm / 1000)
        if (n >= 10000) return "1:" + Math.round(n / 1000) + "k"
        if (n >= 1000)  return "1:" + (n / 1000).toFixed(1) + "k"
        return "1:" + Math.round(n)
    }

    readonly property real barMax: 100        // max bar length in px (also the bar slot width)

    Rectangle {
        anchors { right: parent.right; bottom: parent.bottom; margins: 8 }
        width: row.implicitWidth + 20
        height: 28
        color: hud.bg; border.color: hud.fg; border.width: 1

        Row {
            id: row
            anchors.centerIn: parent
            spacing: 2

            Item {
                width: hud.barMax; height: 16
                anchors.verticalCenter: parent.verticalCenter
                Rectangle { x: 0; y: 11; width: hud.barPx; height: 1; color: hud.fg }
                Rectangle { x: 0; y: 6; width: 1; height: 6; color: hud.fg }
                Rectangle { x: hud.barPx - 1; y: 6; width: 1; height: 6; color: hud.fg }
            }

            Text {
                width: 44
                text: hud.distText(hud.niceDist); color: hud.fg
                horizontalAlignment: Text.AlignRight
                font { family: "Hack"; pixelSize: 12; bold: true }
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                width: 52
                text: hud.ratioText(); color: "#888"
                horizontalAlignment: Text.AlignRight
                font { family: "Hack"; pixelSize: 12 }
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }
}