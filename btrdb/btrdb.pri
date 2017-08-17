QT += qml quick
CONFIG += qt plugin c++11

INCLUDEPATH += $$PWD

SOURCES += \
    $$PWD/btrdb-cpp/btrdb.pb.cc \
    $$PWD/btrdb-cpp/btrdb.grpc.pb.cc \
    $$PWD/btrdb-cpp/btrdb_util.cpp \
    $$PWD/btrdb-cpp/btrdb_mash.cpp \
    $$PWD/btrdb-cpp/btrdb_endpoint.cpp \
    $$PWD/btrdb-cpp/btrdb_stream.cpp \
    $$PWD/btrdb-cpp/btrdb.cpp

HEADERS += \
    $$PWD/btrdb-cpp/btrdb.pb.h \
    $$PWD/btrdb-cpp/btrdb.grpc.pb.h \
    $$PWD/btrdb-cpp/btrdb_util.h \
    $$PWD/btrdb-cpp/btrdb_mash.h \
    $$PWD/btrdb-cpp/btrdb_endpoint.h \
    $$PWD/btrdb-cpp/btrdb_stream.h \
    $$PWD/btrdb-cpp/btrdb.h
