QT += core widgets

TARGET = checkers
TEMPLATE = app

CONFIG += c++11

SOURCES += \
    game/gamecomp.cpp \
    main.cpp \
    game/gamelogic.cpp \
    ui/checkerboard.cpp \
    ui/mainwindow.cpp \
    ui/selectmodwindow.cpp

HEADERS += \
    game/gamecomp.h \
    game/gamelogic.h \
    ui/checkerboard.h \
    ui/mainwindow.h \
    ui/selectmodwindow.h