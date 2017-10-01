QT *= network
qpmx_static: QT += gui widgets

PUBLIC_HEADERS += \
	$$PWD/QSingleInstance/qsingleinstance.h

HEADERS += $$PUBLIC_HEADERS \
	$$PWD/QSingleInstance/clientinstance.h \
	$$PWD/QSingleInstance/qsingleinstance_p.h

SOURCES += \
	$$PWD/QSingleInstance/qsingleinstance.cpp \
	$$PWD/QSingleInstance/clientinstance.cpp \
	$$PWD/QSingleInstance/qsingleinstance_p.cpp

INCLUDEPATH += $$PWD/QSingleInstance
