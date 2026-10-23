import QtQuick 2.15
import QtQuick.Layouts
import terra

Rectangle {
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: parent.top
    height: 30
    color: "transparent"

    readonly property var allowedFixedSize: ["logo", "time", "gps", "pressure", "battery"]

    SystemStatus { id: sys }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 15
        anchors.rightMargin: 15
        spacing: 15

        Repeater {
            model: [
                { type: "logo", label: "FALQUON//TERRA" },
                { type: "gps", label: "GPS 3D 00sat ±0m"},
                { type: "altitude", label: "ALT 0000m"},
                { type: "pressure", label: "BARO 000hPa 0/h"},
                { type: "temperature", label: "TEMP 0°C"},
                { type: "connection", label: sys.online ? "ONLINE" : "OFF-GRID"},
                { type: "weather", label: "WX CALM"},
                { type: "battery", label: (() => {
                        let bat = Math.max(0, Math.min(100, sys.battery));

                        let filledCount = (bat >= 90) ? 5 : Math.floor(bat / 20);

                        let filled = "█████".substring(0, filledCount);
                        let empty = "░░░░░".substring(filledCount);

                        return `BAT ${sys.battery}% ${filled + empty}`;
                    })()
                },
                { type: "time", label: sys.time }
            ]

            Rectangle {
                Layout.fillWidth: allowedFixedSize.includes(modelData.type) ? false : true
                Layout.preferredWidth: allowedFixedSize.includes(modelData.type) ? contentText.implicitWidth + 10 : 0
                Layout.preferredHeight: contentText.implicitHeight + 4

                color: modelData.type === "weather" ? Theme.highlight : "transparent"

                Text {
                    id: contentText
                    anchors.centerIn: parent
                    horizontalAlignment: Text.AlignHCenter

                    font.family: 'Hack'
                    font.weight: {
                        if (modelData.type === "time" || modelData.type === "logo" || modelData.type === "weather")  return 700
                        else return 400
                    }
                    font.pixelSize: 13
                    text: modelData.label
                    color: {
                        if (modelData.type === "weather") return Theme.background;
                        if (modelData.type !== "battery") return Theme.highlight;
                        if (sys.battery <= 20 && sys.battery > 10) return Theme.alert;
                        if (sys.battery <= 10) return Theme.danger;
                        return Theme.highlight;
                    }
                }
            }
        }
    }
}