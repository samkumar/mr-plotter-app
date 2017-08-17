import QtQuick 2.0
import QtQuick.Controls 2.2

CheckBox {
    property real dimension

    id: root

    indicator.implicitWidth: dimension
    indicator.implicitHeight: dimension

    Component.onCompleted: {
        var checkdim = dimension * 2 / 3;
        root.children[0].width = checkdim;
        root.children[0].height = checkdim;
    }
}
