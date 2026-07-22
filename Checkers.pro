QT += core widgets network

TARGET = checkers
TEMPLATE = app

CONFIG += c++11

SOURCES += \
    game/gamecomp.cpp \
    main.cpp \
    game/gamelogic.cpp \
    network/client.cpp \
    network/server.cpp \
    ui/checkerboard.cpp \
    ui/mainwindow.cpp \
    ui/networkwindow.cpp \
    ui/selectmodwindow.cpp

HEADERS += \
    game/gamecomp.h \
    game/gamelogic.h \
    network/client.h \
    network/protocol.h \
    network/server.h \
    ui/checkerboard.h \
    ui/mainwindow.h \
    ui/networkwindow.h \
    ui/selectmodwindow.h

win32 {
    QMAKE_CXXFLAGS += -municode
    QMAKE_LFLAGS += -municode
}