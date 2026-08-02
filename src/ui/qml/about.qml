import QtQuick
import QtQuick.Layouts
import "components"

Item {
    anchors.fill: parent

    MapleFontLoader {
        id: mapleFont
    }

    FontLoader {
        id: mapleExtraBoldItalic
        source: "qrc:/qt/qml/src/ui/qml/assets/fonts/MapleMono-NF-ExtraBoldItalic.ttf"
    }

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 20

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: "K1NG PRO (4K) Driver"
            color: "#f2cdcd"
            font.pixelSize: 36
            font.family: mapleExtraBoldItalic.name
            font.italic: true

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: Qt.openUrlExternally("https://git.vacpro.fyi/moxiu/K1NG-Driver/src/branch/dev/src/ui")
            }
        }

        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 8

            Row {
                Layout.alignment: Qt.AlignHCenter
                spacing: 6

                Text {
                    text: "main repo:"
                    color: "#6c7086"
                    font.pixelSize: 13
                    font.family: mapleFont.name
                }

                Text {
                    text: "git.vacpro.fyi:moxiu/K1NG-Driver"
                    color: "#cba6f7"
                    font.pixelSize: 13
                    font.family: mapleFont.name
                    font.underline: true

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: Qt.openUrlExternally("https://git.vacpro.fyi/moxiu/K1NG-Driver")
                    }
                }
            }

            Row {
                Layout.alignment: Qt.AlignHCenter
                spacing: 6

                Text {
                    text: "mirror repo:"
                    color: "#6c7086"
                    font.pixelSize: 13
                    font.family: mapleFont.name
                }

                Text {
                    text: "github:cfels/K1NG-Driver"
                    color: "#cba6f7"
                    font.pixelSize: 13
                    font.family: mapleFont.name
                    font.underline: true

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: Qt.openUrlExternally("https://github.com/cfels/K1NG-Driver")
                    }
                }
            }
        }
    }
}
