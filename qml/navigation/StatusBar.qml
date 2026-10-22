import QtQuick 2.15
import QtQuick.Layouts // Required for RowLayout and Layout properties
import terra

Rectangle {
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: parent.top
    height: 30
    color: "transparent"

    SystemStatus { id: sys }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 15
        anchors.rightMargin: 15 // Ensure this matches leftMargin
        spacing: 10

        Repeater {
            model: [
                { type: "logo", label: "FALQUON//TERRA" },
                { type: "info", label: "GPS 3D 00sat ±0m"},
                { type: "info", label: "ALT 0000m"},
                { type: "info", label: "BARO 000hPa 0/h"},
                { type: "info", label: "TEMP 0°C"},
                { type: "weather", label: "WX CAUTION"},
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
                Layout.preferredWidth: contentText.implicitWidth + 10
                Layout.preferredHeight: contentText.implicitHeight + 4
                Layout.fillWidth: (index > 0 && index < parent.parent.itemsModel.length - 1)

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
                        else return Theme.highlight;
                    }
                }
            }
        }
    }
}