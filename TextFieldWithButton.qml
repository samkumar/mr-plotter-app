import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

RowLayout {
    property string defaultText;
    property string buttonText;
    property string toolTipText;
    property var onButtonClicked;

    property alias enteredText: inputbox.text;

    spacing: 2

    TextField {
        id: inputbox
        placeholderText: defaultText;
        Layout.fillWidth: true
        ToolTip {
            delay: 1000
            text: toolTipText;
            visible: parent.hovered
        }
    }
    Button {
        text: buttonText

        background.implicitWidth: Math.max(background.implicitHeight, contentItem.implicitWidth)
        onClicked: onButtonClicked(enteredText)
    }
}
