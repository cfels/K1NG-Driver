import QtQuick
import QtQuick.Window
import md3.Core

Window {
    id: appWindow
    property int maxContentWidth: 1000
    property int maxContentHeight: 600

    width: maxContentWidth
    height: maxContentHeight
    visible: true
    color: "transparent"
    flags: Qt.FramelessWindowHint
    title: "K1NG UI"

    Repeater {
        model: 4
        Rectangle {
            anchors.centerIn: surface
            width: surface.width + (index + 1) * 12
            height: surface.height + (index + 1) * 12
            radius: surface.radius + (index + 1) * 6
            color: "transparent"
            border.width: 8
            border.color: Qt.rgba(0, 0, 0, 0.05 - index * 0.01)
        }
    }

    Rectangle {
        id: surface
        anchors.centerIn: parent
        width: 200
        height: 200
        radius: 26
        color: "#11111b"

        property bool controlsVisible: false

        Behavior on width {
            NumberAnimation {
                duration: 500
                easing.type: Easing.InOutCubic
                onRunningChanged: if (!running && surface.width === appWindow.maxContentWidth) surface.controlsVisible = true
            }
        }
        Behavior on height {
            NumberAnimation { duration: 500; easing.type: Easing.InOutCubic }
        }

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            onPressed: appWindow.startSystemMove()
        }

        Loading {
            anchors.centerIn: parent
            opacity: surface.controlsVisible ? 0 : 1
            Behavior on opacity { NumberAnimation { duration: 200 } }
        }

        Loader {
            id: pageLoader
            anchors.fill: parent
            opacity: 0
            source: surface.controlsVisible ? "mainpage.qml" : ""
            onStatusChanged: if (status === Loader.Ready) fadeIn.start()
            NumberAnimation on opacity {
                id: fadeIn
                to: 1
                duration: 250
                easing.type: Easing.OutCubic
            }
        }
    }

    Timer {
        interval: 3000
        running: true
        repeat: false
        onTriggered: {
            surface.width = maxContentWidth
            surface.height = maxContentHeight
        }
    }
}
