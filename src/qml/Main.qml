import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window

    visible: true
    width: 1120
    height: 720
    minimumWidth: 860
    minimumHeight: 560
    title: "Omacam"
    color: Theme.background

    property bool settingsVisible: false
    property string toastText: ""
    property bool toastError: false

    function showToast(text, isError) {
        toastText = text
        toastError = isError
        toastTimer.restart()
    }

    Timer {
        id: toastTimer
        interval: 2600
        onTriggered: window.toastText = ""
    }

    Shortcut {
        sequence: "Space"
        enabled: !settingsPanel.editing
        onActivated: CameraModel.takePhoto()
    }

    Shortcut {
        sequence: "P"
        enabled: !settingsPanel.editing
        onActivated: CameraModel.openPhotosDir()
    }

    Shortcut {
        sequence: "R"
        enabled: !settingsPanel.editing
        onActivated: CameraModel.recording ? CameraModel.stopRecording() : CameraModel.startRecording()
    }

    Shortcut {
        sequence: "V"
        enabled: !settingsPanel.editing
        onActivated: CameraModel.openVideosDir()
    }

    Shortcut {
        sequence: "T"
        enabled: !settingsPanel.editing
        onActivated: CameraModel.cycleTimer()
    }

    Shortcut {
        sequence: "M"
        enabled: !settingsPanel.editing
        onActivated: CameraModel.mirror = !CameraModel.mirror
    }

    Shortcut {
        sequence: "S"
        enabled: !settingsPanel.editing
        onActivated: window.settingsVisible = !window.settingsVisible
    }

    Shortcut {
        sequence: "F11"
        onActivated: window.visibility === Window.FullScreen ? window.showNormal() : window.showFullScreen()
    }

    Shortcut {
        sequence: "Esc"
        onActivated: {
            if (window.visibility === Window.FullScreen)
                window.showNormal()
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        PreviewPane {
            id: previewPane
            Layout.fillWidth: true
            Layout.fillHeight: true
            settingsOpen: window.settingsVisible
            onSettingsToggled: window.settingsVisible = !window.settingsVisible
        }

        SettingsPanel {
            id: settingsPanel
            Layout.preferredWidth: 330
            Layout.fillHeight: true
            visible: window.settingsVisible
            onCloseRequested: window.settingsVisible = false
        }
    }

    Rectangle {
        id: toast

        visible: window.toastText.length > 0
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 110
        width: toastLabel.implicitWidth + 28
        height: 38
        radius: 10
        color: window.toastError ? Theme.danger : Theme.surfaceAlt

        Text {
            id: toastLabel
            anchors.centerIn: parent
            text: window.toastText
            color: window.toastError ? "white" : Theme.foreground
            font.pixelSize: 12
        }
    }

    Connections {
        target: CameraModel
        function onMessage(text, isError) {
            window.showToast(text, isError)
        }
    }

    Component.onCompleted: {
        if (CameraModel.startupWarning.length > 0)
            window.showToast(CameraModel.startupWarning, false)
    }
}
