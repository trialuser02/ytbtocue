TEMPLATE = app
QT += widgets
TARGET = ytbtocue

FORMS += mainwindow.ui

SOURCES += main.cpp \
    mainwindow.cpp \
    cuemodel.cpp \
    utils.cpp

HEADERS += \
    mainwindow.h \
    cuemodel.h \
    utils.h
