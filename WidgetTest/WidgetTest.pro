#-------------------------------------------------
#
# Project created by QtCreator 2015-10-23T16:34:58
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = QSingleInstance
TEMPLATE = app

include(../QSingleInstance/qsingleinstance.pri)

SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui

CONFIG += mobility
MOBILITY = 

