import QtQuick 2.5
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2

Window {
    visible: true

    Connections {
        target: instance
        onInstanceMessage: {
            args.open();
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            Qt.quit();
        }
    }

    Text {
        text: qsTr("Hello World")
        anchors.centerIn: parent
    }

    MessageDialog {
        id: args
        title: "Message"
        text: "New Instance just started!"
        icon: StandardIcon.Information
    }
}

