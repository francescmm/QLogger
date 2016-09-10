QT       -= gui

TARGET = QLogger
TEMPLATE = lib
CONFIG += static

SOURCES += QLogger.cpp

HEADERS += QLogger.h

QMAKE_CXXFLAGS += -std=c++11
