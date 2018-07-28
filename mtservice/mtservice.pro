QT -= core
QT -= gui

CONFIG -= qt
CONFIG += qtc_runnable
CONFIG += c++1z

CONFIG += qt dll debug_and_release
equals(QMAKE_CXX, g++) {
    message("enabling c++17 support")
    QMAKE_CXXFLAGS += -std=gnu++17
}

QMAKE_CXXFLAGS += -fno-stack-protector

TARGET = testservice
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp

HEADERS += mtservice.hpp

INCLUDEPATH += /usr/include/boost

LIBS -= -lQtGui
LIBS -= -lQtCore
LIBS += -lpthread
LIBS += -L/usr/local/lib -lboost_system
LIBS += -L/usr/local/lib -lboost_filesystem
LIBS += -L/usr/local/lib -lboost_chrono
LIBS += -L/usr/local/lib -lboost_thread
LIBS += -L/usr/local/lib -lboost_timer
LIBS += -L/usr/local/lib -lboost_iostreams

CONFIG(debug, debug|release) {
    DESTDIR = $$PWD/Debug
}
CONFIG(release, debug|release) {
    DESTDIR = $$PWD/Release
}

OBJECTS_DIR = $$DESTDIR
MOC_DIR = $$DESTDIR
RCC_DIR = $$DESTDIR
UI_DIR = $$DESTDIR
