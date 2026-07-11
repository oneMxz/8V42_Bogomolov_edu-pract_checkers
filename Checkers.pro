QT += core widgets

TARGET = checkers
TEMPLATE = app

CONFIG += c++11

SOURCES += \
    main.cpp \
    game/gamelogic.cpp \
    ui/checkerboard.cpp

HEADERS += \
    game/gamelogic.h \
    ui/checkerboard.h