import QtQuick
import QtQuick.Controls

Column {
    id: root

    property string label: ""
    property alias model: combo.model
    property alias textRole: combo.textRole
    property alias valueRole: combo.valueRole
    property alias currentIndex: combo.currentIndex
    property alias currentValue: combo.currentValue
    signal activated(int index)

    width: parent ? parent.width : implicitWidth
    spacing: 6

    Text {
        text: root.label
        color: Theme.muted
        font.pixelSize: 12
        Accessible.ignored: true
    }

    ComboBox {
        id: combo
        width: root.width
        font.pixelSize: 13
        enabled: root.enabled
        Accessible.name: root.label

        palette.text: Theme.foreground
        palette.buttonText: Theme.foreground
        palette.button: Theme.surfaceAlt
        palette.base: Theme.surfaceAlt
        palette.window: Theme.surface
        palette.highlight: Theme.accent
        palette.highlightedText: Theme.background
        palette.mid: Theme.surfaceAlt
        palette.dark: Theme.background

        onActivated: root.activated(index)
    }
}
