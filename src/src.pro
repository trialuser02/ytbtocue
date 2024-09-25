include(../ytbtocue.pri)

TEMPLATE = app
QT += widgets
TARGET = ytbtocue

FORMS += mainwindow.ui \
    settingsdialog.ui

SOURCES += main.cpp \
    mainwindow.cpp \
    cuemodel.cpp \
    settingsdialog.cpp \
    utils.cpp \
    tracklistitemdelegate.cpp

HEADERS += \
    mainwindow.h \
    cuemodel.h \
    settingsdialog.h \
    utils.h \
    tracklistitemdelegate.h

RESOURCES += translations/translations.qrc

target.path = $$BINDIR

desktop.files = ytbtocue.desktop
desktop.path = $$DATADIR/applications

INSTALLS += target desktop
