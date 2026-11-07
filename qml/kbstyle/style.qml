import QtQuick
import QtQuick.VirtualKeyboard
import QtQuick.VirtualKeyboard.Styles

KeyboardStyle {
    id: style

    readonly property color fg: "white"
    readonly property color bg: "black"
    readonly property color dim: "#666666"

    keyboardDesignWidth: 2560
    keyboardDesignHeight: 560
    keyboardRelativeLeftMargin: 0
    keyboardRelativeRightMargin: 0
    keyboardRelativeTopMargin: 0
    keyboardRelativeBottomMargin: 0

    component Key: KeyPanel {
        id: kp
        property string label: ""

        Rectangle {
            anchors.fill: parent
            color: control.pressed ? style.fg : style.bg
            border.color: style.dim
            border.width: 1

            Text {
                anchors.centerIn: parent
                text: kp.label !== "" ? kp.label : control.displayText
                color: control.pressed ? style.bg : style.fg
                font.family: "Hack"
                font.weight: 700
                font.pixelSize: 44 * style.scaleHint
            }
        }
    }

    keyboardBackground: Rectangle {
        color: style.bg
        Rectangle { width: parent.width; height: 1; color: style.fg }
    }

    keyPanel: Key {}
    shiftKeyPanel: Key { label: "SHIFT" }
    enterKeyPanel: Key { label: "ENTER" }
    backspaceKeyPanel: Key { label: "DEL" }
    spaceKeyPanel: Key {}
    symbolKeyPanel: Key {}
    modeKeyPanel: Key {}
    languageKeyPanel: Key { label: "LANG" }
    handwritingKeyPanel: Key { label: "HW" }
    hideKeyPanel: Key { label: "HIDE" }

    // key preview and long-press popup
    characterPreviewMargin: 0
    characterPreviewDelegate: Item {
        property string text
        Rectangle {
            anchors.fill: parent
            color: style.fg
            Text {
                anchors.centerIn: parent
                text: parent.parent.text
                color: style.bg
                font.family: "Hack"; font.weight: 700
                font.pixelSize: 80 * style.scaleHint
            }
        }
    }

    alternateKeysListItemWidth: 99 * scaleHint
    alternateKeysListItemHeight: 150 * scaleHint
    alternateKeysListBackground: Rectangle { color: style.bg; border.color: style.fg; border.width: 1 }
    alternateKeysListHighlight: Rectangle { color: style.dim }
    alternateKeysListDelegate: Item {
        width: style.alternateKeysListItemWidth
        height: style.alternateKeysListItemHeight
        Text {
            anchors.centerIn: parent
            text: model.text
            color: style.fg
            font.family: "Hack"; font.pixelSize: 60 * style.scaleHint
        }
    }

    // word suggestions (hidden while predictive text is off)
    selectionListHeight: 85 * scaleHint
    selectionListBackground: Rectangle { color: style.bg }
    selectionListHighlight: Rectangle { color: style.dim }
    selectionListDelegate: SelectionListItem {
        width: label.width + 120 * style.scaleHint
        Text {
            id: label
            anchors.centerIn: parent
            text: display
            color: style.fg
            font.family: "Hack"; font.pixelSize: 44 * style.scaleHint
        }
    }
}