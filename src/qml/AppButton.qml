import QtQuick

Rectangle {
    id: root

    property string text: ""
    property bool active: false
    property bool danger: false
    property bool subtle: false
    signal clicked()

    implicitWidth: label.implicitWidth + 26
    implicitHeight: 34
    radius: 8
    opacity: enabled ? 1.0 : 0.45
    activeFocusOnTab: enabled
    Accessible.role: Accessible.Button
    Accessible.name: root.text

    color: {
        if (root.danger && (root.active || mouseArea.containsMouse))
            return Qt.darker(Theme.danger, 1.15)
        if (root.active)
            return Theme.accent
        if (mouseArea.containsMouse)
            return Qt.lighter(Theme.surfaceAlt, 1.3)
        return root.subtle ? "transparent" : Theme.surfaceAlt
    }
    border.width: root.activeFocus ? 2 : (root.subtle ? 1 : 0)
    border.color: root.activeFocus ? Theme.accent : Qt.lighter(Theme.surfaceAlt, 1.5)

    Text {
        id: label
        anchors.centerIn: parent
        text: root.text
        color: root.active ? Theme.background : Theme.foreground
        font.pixelSize: 13
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: {
            root.clicked()
            root.forceActiveFocus()
        }
    }

    Keys.onReturnPressed: root.clicked()
    Keys.onEnterPressed: root.clicked()
}
