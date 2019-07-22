#Some conf to redirect intermediate stuff in separate dirs
UI_DIR=./.build/ui/
MOC_DIR=./.build/moc/
OBJECTS_DIR=./.build/obj
RCC_DIR=./.build/rcc

QMAKE_DISTCLEAN += -r .build
QMAKE_DISTCLEAN += translations/*.qm

CONFIG += c++11 hide_symbols
DEFINES += QT_DEPRECATED_WARNINGS
QMAKE_DISTCLEAN += -r .build

#Install paths
unix {
  isEmpty(PREFIX) {
    PREFIX = /usr
  }

  BINDIR = $$PREFIX/bin
  DATADIR = $$PREFIX/share
}
