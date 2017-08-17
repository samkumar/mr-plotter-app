import QtQuick 2.0
import QtQuick.Controls 2.2

Button {
    property string buttonText
    property string buttonToolTip

    id: root
    text: buttonText

    background.implicitWidth: contentItem.implicitWidth

    ToolTip {
        delay: 1000
        text: buttonToolTip
        visible: parent.hovered
    }
}
