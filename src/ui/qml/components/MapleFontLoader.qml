import QtQuick

Item {
    id: root
    visible: false

    readonly property alias name: mapleFont.name

    FontLoader {
        id: mapleFont
        source: "qrc:/qt/qml/src/ui/qml/assets/fonts/MapleMono-NF-Regular.ttf"
    }
}
