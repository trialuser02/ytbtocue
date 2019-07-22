include(../ytbtocue.pri)

TEMPLATE = app
QT += widgets
TARGET = ytbtocue

FORMS += mainwindow.ui

SOURCES += main.cpp \
    mainwindow.cpp \
    cuemodel.cpp \
    utils.cpp \
    tracklistitemdelegate.cpp

HEADERS += \
    mainwindow.h \
    cuemodel.h \
    utils.h \
    tracklistitemdelegate.h

RESOURCES += translations/translations.qrc

target.path = $$BINDIR

INSTALLS += target
