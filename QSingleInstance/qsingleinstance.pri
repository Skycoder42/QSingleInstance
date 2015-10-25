QT *= network
CONFIG *= C++11

HEADERS += \
    $$PWD/qsingleinstance.h \
    $$PWD/qsingleinstanceprivate.h \
    $$PWD/clientinstance.h

SOURCES += \
    $$PWD/qsingleinstance.cpp \
    $$PWD/qsingleinstanceprivate.cpp \
    $$PWD/clientinstance.cpp

INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
