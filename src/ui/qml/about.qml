import QtQuick
import QtQuick.Layouts
import com.moxiu.k1ng 1.0
import "components"

Item {
    anchors.fill: parent

    DriverAPI {
        id: driver
    }

    MapleFontLoader {
        id: mapleFont
    }

    FontLoader {
        id: mapleBoldItalic
        source: "qrc:/qt/qml/src/ui/qml/assets/fonts/MapleMono-NF-BoldItalic.ttf"
    }

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 20

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: "K1NG PRO (4K) Driver"
            color: "#f2cdcd"
            font.pixelSize: 36
            font.family: mapleBoldItalic.name
            font.bold: true
            font.italic: true
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
                    text: "<a href='https://git.vacpro.fyi/moxiu/K1NG-Driver' style='color:#f5c2e7;text-decoration:none;'>git.vacpro.fyi:moxiu/K1NG-Driver</a>"
                    textFormat: Text.RichText
                    font.pixelSize: 13
                    font.family: mapleFont.name
                    onLinkActivated: link => driver.openUrl(link)
                    HoverHandler {
                        cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
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
                    text: "<a href='https://github.com/cfels/K1NG-Driver' style='color:#f5c2e7;text-decoration:none;'>github:cfels/K1NG-Driver</a>"
                    textFormat: Text.RichText
                    font.pixelSize: 13
                    font.family: mapleFont.name
                    onLinkActivated: link => driver.openUrl(link)
                    HoverHandler {
                        cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                    }
                }
            }
        }
    }
}
