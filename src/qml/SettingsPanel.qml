import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: panel

    signal closeRequested()

    readonly property bool editing: photosField.activeFocus || videosField.activeFocus

    color: Theme.surface

    function syncCombos() {
        var i
        var index = -1
        for (i = 0; i < CameraModel.devices.length; i++) {
            if (CameraModel.devices[i].id === CameraModel.deviceId) {
                index = i
                break
            }
        }
        deviceCombo.currentIndex = index

        index = -1
        for (i = 0; i < CameraModel.resolutions.length; i++) {
            if (CameraModel.resolutions[i].value === CameraModel.resolution) {
                index = i
                break
            }
        }
        resolutionCombo.currentIndex = index

        index = CameraModel.pixelFormats.indexOf(CameraModel.pixelFormat)
        pixelCombo.currentIndex = index

        index = -1
        for (i = 0; i < CameraModel.frameRates.length; i++) {
            if (Math.abs(CameraModel.frameRates[i].value - CameraModel.frameRate) < 0.01) {
                index = i
                break
            }
        }
        frameRateCombo.currentIndex = index

        timerCombo.currentIndex = [0, 3, 10].indexOf(CameraModel.timerSeconds)
    }

    Component.onCompleted: syncCombos()

    Connections {
        target: CameraModel
        function onDevicesChanged() { panel.syncCombos() }
        function onDeviceChanged() { panel.syncCombos() }
        function onFormatsChanged() { panel.syncCombos() }
        function onSelectionChanged() { panel.syncCombos() }
        function onTimerSecondsChanged() { panel.syncCombos() }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 14

        RowLayout {
            Layout.fillWidth: true

            Text {
                Layout.fillWidth: true
                text: "Settings"
                color: Theme.foreground
                font.pixelSize: 18
                font.bold: true
            }

            AppButton {
                text: "Close"
                subtle: true
                onClicked: panel.closeRequested()
            }
        }

        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: width
            contentHeight: content.implicitHeight

            ColumnLayout {
                id: content
                width: parent.width
                spacing: 14

                SettingCombo {
                    id: deviceCombo
                    Layout.fillWidth: true
                    label: "Camera"
                    enabled: !CameraModel.recording
                    model: CameraModel.devices
                    textRole: "label"
                    valueRole: "id"
                    onActivated: {
                        CameraModel.selectDevice(currentValue)
                        panel.syncCombos()
                    }
                }

                SettingCombo {
                    id: resolutionCombo
                    Layout.fillWidth: true
                    label: "Resolution"
                    enabled: !CameraModel.recording
                    model: CameraModel.resolutions
                    textRole: "label"
                    valueRole: "value"
                    onActivated: CameraModel.setResolution(currentValue)
                }

                SettingCombo {
                    id: pixelCombo
                    Layout.fillWidth: true
                    label: "Pixel format"
                    enabled: !CameraModel.recording
                    model: CameraModel.pixelFormats
                    onActivated: CameraModel.setPixelFormat(currentText)
                }

                SettingCombo {
                    id: frameRateCombo
                    Layout.fillWidth: true
                    label: "Frame rate"
                    enabled: !CameraModel.recording
                    model: CameraModel.frameRates
                    textRole: "label"
                    valueRole: "value"
                    onActivated: CameraModel.setFrameRate(currentValue)
                }

                SettingCombo {
                    id: timerCombo
                    Layout.fillWidth: true
                    label: "Timer"
                    model: [
                        { value: 0, label: "Off" },
                        { value: 3, label: "3 s" },
                        { value: 10, label: "10 s" }
                    ]
                    textRole: "label"
                    valueRole: "value"
                    onActivated: CameraModel.timerSeconds = currentValue
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: Theme.surfaceAlt
                }

                RowLayout {
                    Layout.fillWidth: true

                    Text {
                        Layout.fillWidth: true
                        text: "Microphone"
                        color: Theme.foreground
                        font.pixelSize: 13
                    }

                    Switch {
                        checked: CameraModel.micEnabled
                        enabled: CameraModel.micAvailable && !CameraModel.recording
                        Accessible.name: "Microphone"
                        palette.highlight: Theme.accent
                        palette.text: Theme.foreground
                        palette.base: Theme.surfaceAlt
                        onToggled: CameraModel.micEnabled = checked
                    }
                }

                Text {
                    Layout.fillWidth: true
                    visible: !CameraModel.micAvailable
                    text: "No microphone detected; video will record silently."
                    color: Theme.muted
                    font.pixelSize: 11
                    wrapMode: Text.WordWrap
                }

                RowLayout {
                    Layout.fillWidth: true

                    Text {
                        Layout.fillWidth: true
                        text: "Mirror preview"
                        color: Theme.foreground
                        font.pixelSize: 13
                    }

                    Switch {
                        checked: CameraModel.mirror
                        Accessible.name: "Mirror preview"
                        palette.highlight: Theme.accent
                        palette.text: Theme.foreground
                        palette.base: Theme.surfaceAlt
                        onToggled: CameraModel.mirror = checked
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: Theme.surfaceAlt
                }

                Text {
                    text: "Photos folder"
                    color: Theme.muted
                    font.pixelSize: 12
                }

                TextField {
                    id: photosField
                    Layout.fillWidth: true
                    text: CameraModel.photosDir
                    font.pixelSize: 12
                    color: Theme.foreground
                    placeholderTextColor: Theme.muted
                    selectByMouse: true
                    palette.base: Theme.surfaceAlt
                    palette.text: Theme.foreground
                    palette.highlight: Theme.accent
                    background: Rectangle {
                        color: Theme.surfaceAlt
                        radius: 6
                        border.width: 1
                        border.color: Qt.lighter(Theme.surfaceAlt, 1.4)
                    }
                    onEditingFinished: CameraModel.photosDir = text
                }

                AppButton {
                    text: "Open photos folder"
                    subtle: true
                    onClicked: CameraModel.openPhotosDir()
                }

                Text {
                    text: "Videos folder"
                    color: Theme.muted
                    font.pixelSize: 12
                }

                TextField {
                    id: videosField
                    Layout.fillWidth: true
                    text: CameraModel.videosDir
                    font.pixelSize: 12
                    color: Theme.foreground
                    placeholderTextColor: Theme.muted
                    selectByMouse: true
                    palette.base: Theme.surfaceAlt
                    palette.text: Theme.foreground
                    palette.highlight: Theme.accent
                    background: Rectangle {
                        color: Theme.surfaceAlt
                        radius: 6
                        border.width: 1
                        border.color: Qt.lighter(Theme.surfaceAlt, 1.4)
                    }
                    onEditingFinished: CameraModel.videosDir = text
                }

                AppButton {
                    text: "Open videos folder"
                    subtle: true
                    onClicked: CameraModel.openVideosDir()
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: Theme.surfaceAlt
                }

                Text {
                    Layout.fillWidth: true
                    text: "State: " + CameraModel.stateText
                    color: Theme.muted
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                }

                Text {
                    Layout.fillWidth: true
                    visible: CameraModel.errorText.length > 0
                    text: CameraModel.errorText
                    color: Theme.danger
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                }

                Text {
                    Layout.fillWidth: true
                    visible: CameraModel.startupWarning.length > 0
                    text: CameraModel.startupWarning
                    color: Theme.muted
                    font.pixelSize: 11
                    wrapMode: Text.WordWrap
                }

                AppButton {
                    Layout.fillWidth: true
                    text: "Rescan cameras"
                    onClicked: CameraModel.refreshDevices()
                }

                Text {
                    Layout.fillWidth: true
                    text: "Space: photo · R: record · P: photos folder · V: videos folder · T: timer 3/10/off · M: mirror · S: settings · F11: fullscreen"
                    color: Theme.muted
                    font.pixelSize: 11
                    wrapMode: Text.WordWrap
                }
            }
        }
    }
}
