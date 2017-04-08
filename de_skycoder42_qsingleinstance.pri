QT *= network

HEADERS += \
	$$PWD/QSingleInstance/qsingleinstance.h \
	$$PWD/QSingleInstance/clientinstance.h \
	$$PWD/QSingleInstance/qsingleinstance_p.h

SOURCES += \
	$$PWD/QSingleInstance/qsingleinstance.cpp \
	$$PWD/QSingleInstance/clientinstance.cpp \
	$$PWD/QSingleInstance/qsingleinstance_p.cpp

INCLUDEPATH += $$PWD/QSingleInstance
