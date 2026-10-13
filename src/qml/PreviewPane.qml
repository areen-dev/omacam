import QtQuick
import QtMultimedia

Item {
    id: pane

    property bool settingsOpen: false
    signal settingsToggled()

    function formatDuration(milliseconds) {
        var total = Math.floor(milliseconds / 1000)
        var hours = Math.floor(total / 3600)
        var minutes = Math.floor((total % 3600) / 60)
        var seconds = total % 60
        var mm = (minutes < 10 ? "0" : "") + minutes
        var ss = (seconds < 10 ? "0" : "") + seconds
        return hours > 0 ? hours + ":" + mm + ":" + ss : mm + ":" + ss
    }

    Rectangle {
        anchors.fill: parent
        color: "#000000"
    }

    VideoOutput {
        id: preview
        anchors.fill: parent
        fillMode: VideoOutput.PreserveAspectCrop
        transform: Scale {
            origin.x: preview.width / 2
            xScale: CameraModel.mirror ? -1 : 1
        }
    }

    Component.onCompleted: CameraModel.attachVideoOutput(preview.videoSink)

    Connections {
        target: preview.videoSink
        function onVideoFrameChanged(frame) {
            CameraModel.notePreviewFrame()
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: 16
        height: 30
        width: statusRow.implicitWidth + 20
        radius: 8
        color: Qt.rgba(0, 0, 0, 0.55)
        visible: CameraModel.hasCamera && CameraModel.activeFormatLabel.length > 0

        Row {
            id: statusRow
            anchors.centerIn: parent
            spacing: 10

            Text {
                text: CameraModel.activeFormatLabel
                color: "white"
                font.pixelSize: 12
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                text: Math.round(CameraModel.measuredFps) + " fps"
                color: "#b8b8b8"
                font.pixelSize: 12
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    Rectangle {
        visible: CameraModel.recording
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 16
        height: 30
        width: recRow.implicitWidth + 22
        radius: 8
        color: Theme.danger

        Row {
            id: recRow
            anchors.centerIn: parent
            spacing: 8

            Rectangle {
                width: 10
                height: 10
                radius: 5
                color: "white"
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                text: "REC " + pane.formatDuration(CameraModel.recordingDuration)
                color: "white"
                font.pixelSize: 12
                font.bold: true
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    Rectangle {
        visible: CameraModel.timerActive
        anchors.centerIn: parent
        width: 150
        height: 150
        radius: 75
        color: Qt.rgba(0, 0, 0, 0.65)

        Column {
            anchors.centerIn: parent
            spacing: 4

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: CameraModel.timerRemaining
                color: "white"
                font.pixelSize: 72
                font.bold: true
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Tap to cancel"
                color: "#d0d0d0"
                font.pixelSize: 12
            }
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: CameraModel.cancelTimer()
        }
    }

    Rectangle {
        visible: !CameraModel.hasCamera
        anchors.centerIn: parent
        width: emptyColumn.implicitWidth + 48
        height: emptyColumn.implicitHeight + 48
        radius: 16
        color: Qt.rgba(0, 0, 0, 0.55)

        Column {
            id: emptyColumn
            anchors.centerIn: parent
            spacing: 12

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "No camera found"
                color: "white"
                font.pixelSize: 18
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: CameraModel.errorText
                color: "#c8c8c8"
                font.pixelSize: 12
                visible: CameraModel.errorText.length > 0
            }

            AppButton {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Rescan cameras"
                onClicked: CameraModel.refreshDevices()
            }
        }
    }

    Rectangle {
        objectName: "errorBanner"
        visible: CameraModel.hasError
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 56
        height: 38
        width: errorRow.implicitWidth + 24
        radius: 10
        color: Qt.rgba(0, 0, 0, 0.72)

        Row {
            id: errorRow
            anchors.centerIn: parent
            spacing: 12

            Text {
                text: CameraModel.errorText.length > 0 ? CameraModel.errorText : "Camera error"
                color: Theme.danger
                font.pixelSize: 12
                anchors.verticalCenter: parent.verticalCenter
            }

            AppButton {
                text: "Retry"
                anchors.verticalCenter: parent.verticalCenter
                onClicked: CameraModel.retry()
            }
        }
    }

    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 24
        height: 64
        width: controls.implicitWidth + 32
        radius: 20
        color: Qt.rgba(0, 0, 0, 0.62)
        visible: CameraModel.hasCamera

        Row {
            id: controls
            anchors.centerIn: parent
            spacing: pane.width >= 640 ? 16 : 10

            AppButton {
                anchors.verticalCenter: parent.verticalCenter
                visible: pane.width >= 520
                text: "Mirror"
                active: CameraModel.mirror
                onClicked: CameraModel.mirror = !CameraModel.mirror
            }

            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: 48
                height: 48
                radius: 24
                color: shutterMouse.containsMouse && shutterEnabled ? Theme.accent : Theme.foreground
                border.width: 3
                border.color: activeFocus ? Theme.accent : "#000000"
                opacity: shutterEnabled ? 1.0 : 0.45
                activeFocusOnTab: true
                objectName: "shutter"
                Accessible.role: Accessible.Button
                Accessible.name: "Take photo"

                readonly property bool shutterEnabled: CameraModel.imageCaptureReady && !CameraModel.recording

                Keys.onReturnPressed: if (shutterEnabled) CameraModel.takePhoto()
                Keys.onEnterPressed: if (shutterEnabled) CameraModel.takePhoto()

                MouseArea {
                    id: shutterMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: if (parent.shutterEnabled) CameraModel.takePhoto()
                }
            }

            AppButton {
                anchors.verticalCenter: parent.verticalCenter
                text: CameraModel.recording ? "Stop" : "Record"
                danger: true
                active: CameraModel.recording
                onClicked: CameraModel.recording ? CameraModel.stopRecording() : CameraModel.startRecording()
            }

            AppButton {
                anchors.verticalCenter: parent.verticalCenter
                text: "Settings"
                active: pane.settingsOpen
                onClicked: pane.settingsToggled()
            }

            AppButton {
                anchors.verticalCenter: parent.verticalCenter
                visible: pane.width >= 640
                text: "Photos"
                subtle: true
                onClicked: CameraModel.openPhotosDir()
            }

            AppButton {
                anchors.verticalCenter: parent.verticalCenter
                visible: pane.width >= 640
                text: "Videos"
                subtle: true
                onClicked: CameraModel.openVideosDir()
            }
        }
    }
}
