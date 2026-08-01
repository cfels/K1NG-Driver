import QtQuick
import QtQuick.Layouts
import md3.Core
import com.moxiu.k1ng 1.0

Item {
    id: root
    anchors.fill: parent

    property int dpi: 800
    property int currentTab: 0

    readonly property color crust: "#11111b"
    readonly property color base: "#1e1e2e"
    readonly property color mantle: "#181825"
    readonly property color surface: "#313244"
    readonly property color overlay: "#6c7086"
    readonly property color text: "#cdd6f4"
    readonly property color subtext: "#bac2de"
    readonly property color accent: "#cba6f7"
    readonly property color red: "#f38ba8"
    readonly property color yellow: "#f9e2af"

    DriverAPI {
        id: driver
    }

    PresetManager {
        id: presets
    }

    Rectangle {
        anchors.fill: parent
        color: base
        radius: 26
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Item {
            Layout.fillWidth: true
            height: 72

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                onPressed: appWindow.startSystemMove()
            }

            Row {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.rightMargin: 18
                spacing: 10

                Rectangle {
                    width: 18
                    height: 18
                    radius: 9
                    anchors.verticalCenter: parent.verticalCenter
                    color: minA.containsMouse ? "#fff3c4" : yellow
                    scale: minA.containsMouse ? 1.3 : 1.0
                    Behavior on scale {
                        NumberAnimation {
                            duration: 150
                            easing.type: Easing.OutCubic
                        }
                    }
                    Behavior on color {
                        ColorAnimation {
                            duration: 150
                        }
                    }
                    MouseArea {
                        id: minA
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: appWindow.showMinimized()
                    }
                }

                Rectangle {
                    width: 18
                    height: 18
                    radius: 9
                    anchors.verticalCenter: parent.verticalCenter
                    color: closeA.containsMouse ? "#ffb3c6" : red
                    scale: closeA.containsMouse ? 1.3 : 1.0
                    Behavior on scale {
                        NumberAnimation {
                            duration: 150
                            easing.type: Easing.OutCubic
                        }
                    }
                    Behavior on color {
                        ColorAnimation {
                            duration: 150
                        }
                    }
                    MouseArea {
                        id: closeA
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: appWindow.close()
                    }
                }
            }

            Item {
                anchors.centerIn: parent
                anchors.verticalCenterOffset: 6
                width: tabPill.width
                height: tabPill.height

                Rectangle {
                    id: tabPill
                    width: 260
                    height: 42
                    radius: 21
                    color: surface

                    Rectangle {
                        id: indicator
                        width: tabPill.width / 2 - 5
                        height: tabPill.height - 10
                        radius: (tabPill.height - 10) / 2
                        color: accent
                        y: 5
                        x: root.currentTab === 0 ? 5 : tabPill.width / 2
                        Behavior on x {
                            NumberAnimation {
                                duration: 440
                                easing.type: Easing.OutBack
                                easing.overshoot: 1.5
                            }
                        }
                    }

                    Row {
                        anchors.fill: parent
                        Repeater {
                            model: ["Main", "Settings"]
                            Item {
                                width: tabPill.width / 2
                                height: tabPill.height
                                Text {
                                    anchors.centerIn: parent
                                    text: modelData
                                    color: root.currentTab === index ? base : subtext
                                    font.pixelSize: 14
                                    font.weight: Font.Medium
                                    Behavior on color {
                                        ColorAnimation {
                                            duration: 220
                                        }
                                    }
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: root.currentTab = index
                                    cursorShape: Qt.PointingHandCursor
                                }
                            }
                        }
                    }
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Item {
                anchors.fill: parent
                visible: root.currentTab === 0
                opacity: visible ? 1 : 0
                Behavior on opacity {
                    NumberAnimation {
                        duration: 200
                    }
                }

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 16
                    width: parent.width * 0.72

                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: "DPI: " + root.dpi
                        color: root.text
                        font.pixelSize: 22
                        font.weight: Font.Medium
                    }

                    Slider {
                        id: dpiSlider
                        Layout.fillWidth: true
                        from: 50
                        to: 26000
                        value: 800
                        stepSize: 50
                        snapMode: true
                        valueLabelEnabled: true

                    }

                    Connections {
                        target: dpiSlider
                        function onMoved() {
                            root.dpi = Math.round(dpiSlider.value / 50) * 50;
                        }
                        function onPressedChanged() {
                            if (!dpiSlider.pressed)
                                driver.setDPI(root.dpi);
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        ComboBox {
                            id: presetCombo
                            Layout.fillWidth: true
                            label: "Preset"
                            type: "outlined"
                            model: {
                                var names = [];
                                for (var i = 0; i < presets.presets.length; i++)
                                    names.push(presets.presets[i].name);
                                return names;
                            }
                            currentIndex: -1
                            labelBackgroundColor: root.base

                        }

                        Button {
                            text: "Load"
                            type: "filled"
                            enabled: presetCombo.currentIndex >= 0
                            onClicked: {
                                var p = presets.presets[presetCombo.currentIndex];
                                root.dpi = p.dpi;
                                dpiSlider.value = p.dpi;
                                driver.loadPreset(p.name);
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        TextField {
                            id: presetNameField
                            Layout.fillWidth: true
                            label: "Preset name"
                            type: "outlined"
                            labelBackgroundColor: root.base

                        }

                        Button {
                            text: "Save"
                            type: "filled"
                            enabled: presetNameField.text.trim().length > 0
                            onClicked: {
                                var name = presetNameField.text.trim();
                                presets.savePreset(name, root.dpi);
                                presetNameField.text = "";
                                presets.reload();
                            }
                        }
                    }
                }

                Image {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 16
                    source: "qrc:/qt/qml/src/ui/qml/assets/logo.png"
                    width: 48
                    height: 48
                    fillMode: Image.PreserveAspectFit
                    opacity: 0.4
                }
            }

            Item {
                anchors.fill: parent
                visible: root.currentTab === 1
                opacity: visible ? 1 : 0
                Behavior on opacity {
                    NumberAnimation {
                        duration: 200
                    }
                }

                Loader {
                    anchors.fill: parent
                    source: "settings.qml"
                    active: root.currentTab === 1
                }
            }
        }
    }
}
