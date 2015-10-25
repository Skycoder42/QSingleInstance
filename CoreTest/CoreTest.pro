QT += core
QT -= gui

TARGET = CoreTest
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

include(../QSingleInstance/qsingleinstance.pri)

SOURCES += main.cpp

