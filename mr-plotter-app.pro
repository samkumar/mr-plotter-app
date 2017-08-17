TEMPLATE = app

QT += qml quick
CONFIG += c++11

QMAKE_CXXFLAGS += -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS
#unix:LIBS += -lgrpc++ -lgrpc -lpthread -ldl

# Qt v5.9.1 uses libprotobuf v2.6.1, but libbtrdb uses libprotbuf v3.3.0
# So, I link libbtrdb with its version of libprotobuf statically, and
# Qt's version is linked dynamically. This prevents the versions from
# conflicting at runtime.
#unix:LIBS += -Wl,-Bstatic -lbtrdb -lprotobuf -Wl,-Bdynamic
#unix:LIBS += -Wl,-Bstatic -lprotobuf -Wl,-Bdynamic

# In case you want static linking of gRPC, uncomment this line and comment out the above lines
unix:LIBS += -Wl,-Bstatic -lgrpc++ -lgrpc -lz -lssl -lcrypto -lprotobuf -Wl,-Bdynamic -lpthread -ldl

# For the Windows build, use these lines. Make sure to properly set $$GRPC.
GRPC = C:\Users\Sam\Documents\grpc
win32:INCLUDEPATH += $$GRPC\include
win32:INCLUDEPATH += $$GRPC\third_party\protobuf\src
win32:QMAKE_CXXFLAGS += -D_WIN32_WINNT=0x0600
win32:LIBS += -L$$GRPC\.build -lgrpc++ -lgrpc -lgpr
win32:LIBS += -L$$GRPC\.build\third_party\zlib -lzlib
win32:LIBS += -L$$GRPC\.build\third_party\protobuf -llibprotobuf
win32:LIBS += -L$$GRPC\.build\third_party\boringssl\ssl -lssl
win32:LIBS += -L$$GRPC\.build\third_party\boringssl\crypto -lcrypto
win32:LIBS += -L$$GRPC\.build\third_party\cares -lcares
win32:LIBS += -ladvapi32

SOURCES += main.cpp \
    btrdbstreamtreemodel.cpp \
    mrplotterutils.cpp \
    btrdbdatasource.cpp

RESOURCES += qml.qrc

include(qtlibbw/bosswave.pri)
include(mr-plotter-qml/mrplotter.pri)
include(btrdb/btrdb.pri)

QTPLUGIN += BOSSWAVE

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    btrdbstreamtreemodel.h \
    mrplotterutils.h \
    btrdbdatasource.h
