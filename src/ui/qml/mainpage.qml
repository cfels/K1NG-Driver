import QtQuick
import QtQuick.Layouts
import md3.Core
import com.moxiu.k1ng 1.0
import "components"

Item {
    id: root
    anchors.fill: parent

    property int dpi: 650
    property int currentTab: 0

    // ui font
    MapleFontLoader {
        id: mapleFont
    }
    readonly property string mono: mapleFont.name

    // ui colors
    readonly property color crust: "#11111b"
    readonly property color base: "#1e1e2e"
    readonly property color mantle: "#181825"
    readonly property color surface: "#313244"
    readonly property color overlay: "#6c7086"
    readonly property color text: "#cdd6f4"
    readonly property color subtext: "#bac2de"
    readonly property color accent: "#cba6f7"
    readonly property color pink: "#f5c2e7"
    readonly property color flamingo: "#f2cdcd"
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
        color: crust
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
                propagateComposedEvents: true
                onPressed: mouse => {
                    mouse.accepted = false;
                    appWindow.startSystemMove();
                }
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
                            model: ["Main", "About"]
                            Item {
                                width: tabPill.width / 2
                                height: tabPill.height
                                Text {
                                    anchors.centerIn: parent
                                    text: modelData
                                    color: root.currentTab === index ? crust : subtext
                                    font.pixelSize: 17
                                    font.family: root.mono
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
            id: pageArea
            Layout.fillWidth: true
            Layout.fillHeight: true

            Item {
                id: mainTab
                anchors.fill: parent
                visible: opacity > 0
                opacity: root.currentTab === 0 ? 1.0 : 0.0
                Behavior on opacity {
                    NumberAnimation {
                        duration: 300
                        easing.type: Easing.OutCubic
                    }
                }

                ColumnLayout {
                    id: mainContent
                    anchors.centerIn: parent
                    spacing: 16
                    width: parent.width * 0.72

                    opacity: 0
                    transform: Translate {
                        id: contentDrift
                        y: 16
                    }

                    Component.onCompleted: {
                        contentEnterOpacity.start();
                        contentEnterDrift.start();
                    }

                    NumberAnimation {
                        id: contentEnterOpacity
                        target: mainContent
                        property: "opacity"
                        from: 0
                        to: 1
                        duration: 480
                        easing.type: Easing.OutCubic
                    }
                    NumberAnimation {
                        id: contentEnterDrift
                        target: contentDrift
                        property: "y"
                        from: 16
                        to: 0
                        duration: 520
                        easing.type: Easing.OutCubic
                    }

                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: "DPI: " + root.dpi
                        color: root.text
                        font.pixelSize: 22
                        font.family: root.mono
                        font.weight: Font.Medium
                    }

                    Item {
                        Layout.fillWidth: true
                        height: 44

                        property real _pos: (dpiSlider.value - dpiSlider.from) / (dpiSlider.to - dpiSlider.from)
                        property real _padding: 8
                        property real _avail: width - _padding * 2

                        Rectangle {
                            x: parent._padding + parent._avail * parent._pos + 8
                            anchors.verticalCenter: parent.verticalCenter
                            width: Math.max(0, parent._avail * (1 - parent._pos) - 8)
                            height: 16
                            radius: 8
                            color: root.surface
                        }
                        Rectangle {
                            x: 0
                            anchors.verticalCenter: parent.verticalCenter
                            width: Math.max(0, parent._padding + parent._avail * parent._pos - 4)
                            height: 16
                            radius: 8
                            color: root.accent
                        }
                        Rectangle {
                            x: parent._padding + parent._avail * parent._pos - width / 2
                            anchors.verticalCenter: parent.verticalCenter
                            width: thumbMa.pressed ? 2 : (thumbMa.containsMouse ? 6 : 4)
                            height: 44
                            radius: 2
                            color: root.accent
                            Behavior on width {
                                NumberAnimation {
                                    duration: 150
                                    easing.type: Easing.OutCubic
                                }
                            }
                        }
                        Rectangle {
                            visible: thumbMa.containsMouse || thumbMa.pressed
                            x: parent._padding + parent._avail * parent._pos - 26
                            y: -34
                            width: 56
                            height: 28
                            radius: 14
                            color: root.accent
                            Text {
                                anchors.centerIn: parent
                                text: root.dpi
                                color: root.crust
                                font.pixelSize: 11
                                font.family: root.mono
                                font.weight: Font.Medium
                            }
                        }
                        MouseArea {
                            id: thumbMa
                            anchors.fill: parent
                            anchors.leftMargin: -10
                            anchors.rightMargin: -10
                            hoverEnabled: true
                            preventStealing: true
                            function updateDpi(mx) {
                                var pos = Math.max(0, Math.min(1, (mx - parent._padding) / parent._avail));
                                var s = Math.round((dpiSlider.from + pos * (dpiSlider.to - dpiSlider.from)) / 50) * 50;
                                dpiSlider.value = s;
                                root.dpi = s;
                            }
                            onPressed: m => updateDpi(m.x)
                            onReleased: driver.setDPI(root.dpi)
                            onPositionChanged: m => {
                                if (pressed)
                                    updateDpi(m.x);
                            }
                        }
                        Slider {
                            id: dpiSlider
                            visible: false
                            from: 50
                            to: 26000
                            value: 800
                            stepSize: 50
                            snapMode: true
                        }
                    }

                    RowLayout {
                        id: presetRow
                        Layout.fillWidth: true
                        spacing: 10

                        Rectangle {
                            id: presetField
                            Layout.fillWidth: true
                            height: 56
                            radius: 4
                            color: root.surface

                            MouseArea {
                                anchors.fill: parent
                                z: -1
                                acceptedButtons: Qt.AllButtons
                                onPressed: mouse => mouse.accepted = true
                            }

                            Rectangle {
                                anchors.bottom: parent.bottom
                                anchors.left: parent.left
                                anchors.right: parent.right
                                height: presetComboArea.containsMouse ? 2 : 1
                                color: presetComboArea.containsMouse ? root.pink : root.overlay
                                Behavior on height {
                                    NumberAnimation {
                                        duration: 150
                                    }
                                }
                                Behavior on color {
                                    ColorAnimation {
                                        duration: 150
                                    }
                                }
                            }

                            Text {
                                text: "Preset"
                                color: root.overlay
                                font.pixelSize: presetCombo.currentIndex >= 0 ? 11 : 15
                                font.family: root.mono
                                x: 16
                                y: presetCombo.currentIndex >= 0 ? 8 : (parent.height - height) / 2
                                Behavior on y {
                                    NumberAnimation {
                                        duration: 200
                                        easing.type: Easing.OutCubic
                                    }
                                }
                                Behavior on font.pixelSize {
                                    NumberAnimation {
                                        duration: 200
                                    }
                                }
                            }

                            Text {
                                visible: presetCombo.currentIndex >= 0
                                text: presetCombo.currentIndex >= 0 ? presetCombo.currentText : ""
                                color: root.text
                                font.pixelSize: 15
                                font.family: root.mono
                                x: 16
                                anchors.bottom: parent.bottom
                                anchors.bottomMargin: 10
                                elide: Text.ElideRight
                                width: parent.width - 48
                            }

                            Text {
                                anchors.right: parent.right
                                anchors.rightMargin: 12
                                anchors.verticalCenter: parent.verticalCenter
                                text: "▾"
                                color: root.pink
                                font.pixelSize: 16
                                font.family: root.mono
                                rotation: presetField.menuOpen ? 180 : 0
                                Behavior on rotation {
                                    NumberAnimation {
                                        duration: 200
                                    }
                                }
                            }

                            property bool menuOpen: false

                            MouseArea {
                                id: presetComboArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onPressed: mouse => mouse.accepted = true
                                onClicked: {
                                    presetField.menuOpen = !presetField.menuOpen;
                                    if (presetField.menuOpen)
                                        presetPopup.openPopup();
                                    else
                                        presetPopup.closePopup();
                                }
                            }

                            ComboBox {
                                id: presetCombo
                                visible: false
                                model: {
                                    var names = [];
                                    for (var i = 0; i < presets.presets.length; i++)
                                        names.push(presets.presets[i].name);
                                    return names;
                                }
                                currentIndex: -1
                            }
                        }

                        Rectangle {
                            id: loadBtn
                            width: 64
                            height: 40
                            radius: 20
                            color: presetCombo.currentIndex >= 0 ? (loadMa.pressed ? Qt.darker(root.flamingo, 1.1) : root.flamingo) : Qt.rgba(0.95, 0.80, 0.80, 0.35)
                            Behavior on color {
                                ColorAnimation {
                                    duration: 150
                                }
                            }
                            Text {
                                anchors.centerIn: parent
                                text: "Load"
                                color: presetCombo.currentIndex >= 0 ? root.crust : Qt.rgba(0.95, 0.80, 0.80, 0.5)
                                font.pixelSize: 13
                                font.family: root.mono
                                font.weight: Font.Medium
                            }
                            MouseArea {
                                id: loadMa
                                anchors.fill: parent
                                enabled: presetCombo.currentIndex >= 0
                                cursorShape: Qt.PointingHandCursor
                                onPressed: mouse => mouse.accepted = true
                                onClicked: {
                                    var p = presets.presets[presetCombo.currentIndex];
                                    root.dpi = p.dpi;
                                    dpiSlider.value = p.dpi;
                                    driver.loadPreset(p.name);
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Rectangle {
                            Layout.fillWidth: true
                            height: 56
                            radius: 4
                            color: root.surface

                            MouseArea {
                                anchors.fill: parent
                                z: -1
                                acceptedButtons: Qt.AllButtons
                                onPressed: mouse => mouse.accepted = true
                            }

                            Rectangle {
                                anchors.bottom: parent.bottom
                                anchors.left: parent.left
                                anchors.right: parent.right
                                height: nameInput.activeFocus ? 2 : 1
                                color: nameInput.activeFocus ? root.accent : root.overlay
                                Behavior on height {
                                    NumberAnimation {
                                        duration: 150
                                    }
                                }
                                Behavior on color {
                                    ColorAnimation {
                                        duration: 150
                                    }
                                }
                            }

                            Text {
                                text: "Preset name"
                                color: root.overlay
                                font.pixelSize: nameInput.activeFocus || nameInput.text.length > 0 ? 11 : 15
                                font.family: root.mono
                                x: 16
                                y: nameInput.activeFocus || nameInput.text.length > 0 ? 8 : (parent.height - height) / 2
                                Behavior on y {
                                    NumberAnimation {
                                        duration: 200
                                        easing.type: Easing.OutCubic
                                    }
                                }
                                Behavior on font.pixelSize {
                                    NumberAnimation {
                                        duration: 200
                                    }
                                }
                            }

                            TextInput {
                                id: nameInput
                                x: 16
                                anchors.bottom: parent.bottom
                                anchors.bottomMargin: 10
                                width: parent.width - 32
                                color: root.text
                                font.pixelSize: 15
                                font.family: root.mono
                                selectionColor: root.accent
                                selectedTextColor: root.crust
                                clip: true
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.IBeamCursor
                                    onPressed: mouse => {
                                        mouse.accepted = true;
                                        nameInput.forceActiveFocus();
                                    }
                                }
                            }
                        }

                        Rectangle {
                            width: 64
                            height: 40
                            radius: 20
                            color: nameInput.text.trim().length > 0 ? (saveMa.pressed ? Qt.darker(root.flamingo, 1.1) : root.flamingo) : Qt.rgba(0.95, 0.80, 0.80, 0.35)
                            Behavior on color {
                                ColorAnimation {
                                    duration: 150
                                }
                            }
                            Text {
                                anchors.centerIn: parent
                                text: "Save"
                                color: nameInput.text.trim().length > 0 ? root.crust : Qt.rgba(0.95, 0.80, 0.80, 0.5)
                                font.pixelSize: 13
                                font.family: root.mono
                                font.weight: Font.Medium
                            }
                            MouseArea {
                                id: saveMa
                                anchors.fill: parent
                                enabled: nameInput.text.trim().length > 0
                                cursorShape: Qt.PointingHandCursor
                                onPressed: mouse => mouse.accepted = true
                                onClicked: {
                                    var name = nameInput.text.trim();
                                    presets.savePreset(name, root.dpi);
                                    nameInput.text = "";
                                    presets.reload();
                                }
                            }
                        }
                    }
                }

                Image {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 16
                    source: "qrc:/qt/qml/src/ui/qml/assets/logo.png"
                    width: 60
                    height: 60
                    fillMode: Image.PreserveAspectFit
                    opacity: 0.4
                }
            }

            Item {
                id: aboutTab
                anchors.fill: parent
                visible: opacity > 0
                opacity: root.currentTab === 1 ? 1.0 : 0.0
                Behavior on opacity {
                    NumberAnimation {
                        duration: 300
                        easing.type: Easing.OutCubic
                    }
                }

                Loader {
                    anchors.fill: parent
                    source: "about.qml"
                    active: root.currentTab === 1
                }
            }

            Item {
                id: presetPopup
                visible: false
                opacity: 0
                z: 200
                transformOrigin: Item.TopLeft

                property real _fieldX: 0
                property real _fieldY: 0

                x: _fieldX
                y: _fieldY + presetField.height + 6
                width: presetField.width
                height: Math.min(presetCombo.model.length * 48 + 16, 240)

                function openPopup() {
                    var pt = presetField.mapToItem(pageArea, 0, 0);
                    _fieldX = pt.x;
                    _fieldY = pt.y;
                    visible = true;
                    fadeIn.start();
                    growIn.start();
                }

                function closePopup() {
                    fadeOut.start();
                }

                NumberAnimation {
                    id: fadeIn
                    target: presetPopup
                    property: "opacity"
                    from: 0
                    to: 1
                    duration: 180
                    easing.type: Easing.OutCubic
                }
                NumberAnimation {
                    id: fadeOut
                    target: presetPopup
                    property: "opacity"
                    from: 1
                    to: 0
                    duration: 140
                    easing.type: Easing.InCubic
                    onFinished: {
                        presetPopup.visible = false;
                        presetField.menuOpen = false;
                    }
                }
                NumberAnimation {
                    id: growIn
                    target: presetPopup
                    property: "scale"
                    from: 0.94
                    to: 1
                    duration: 200
                    easing.type: Easing.OutCubic
                }

                MouseArea {
                    id: popupScrim
                    parent: pageArea
                    anchors.fill: parent
                    enabled: presetPopup.visible
                    z: 199
                    onClicked: presetPopup.closePopup()
                }

                Rectangle {
                    anchors.fill: parent
                    radius: 8
                    color: root.pink
                    border.width: 1
                    border.color: Qt.rgba(0, 0, 0, 0.10)
                    clip: true

                    Column {
                        anchors.top: parent.top
                        anchors.topMargin: 8
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 8
                        anchors.left: parent.left
                        anchors.right: parent.right

                        Repeater {
                            model: presetCombo.model
                            Rectangle {
                                width: parent.width
                                height: 48
                                color: rowMa.containsMouse ? Qt.rgba(0, 0, 0, 0.08) : "transparent"
                                Behavior on color {
                                    ColorAnimation {
                                        duration: 100
                                    }
                                }

                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    x: 16
                                    text: modelData
                                    color: root.crust
                                    font.pixelSize: 14
                                    font.family: root.mono
                                    font.weight: Font.Medium
                                }

                                MouseArea {
                                    id: rowMa
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onPressed: mouse => mouse.accepted = true
                                    onClicked: {
                                        presetCombo.currentIndex = index;
                                        presetPopup.closePopup();
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
