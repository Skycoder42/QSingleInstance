#include "mainwindow.h"
#include <QApplication>
#include <qsingleinstance.h>

#include <QMessageBox>
#include <QDebug>

#define USE_EXEC

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	QSingleInstance instance;
	MainWindow *w = NULL;

#ifndef USE_EXEC
	if(instance.process()) {
		if(!instance.isMaster())
			return 0;
	} else
		return -1;
#endif

#ifdef USE_EXEC
	instance.setStartupFunction([&]() -> int {
#endif
		w = new MainWindow(NULL);
		instance.setNotifyWindow(w);
		w->show();
#ifdef USE_EXEC
		return 0;
	});
#endif

	QObject::connect(qApp, &QApplication::aboutToQuit, [&](){
		delete w;
	});

	QObject::connect(&instance, &QSingleInstance::instanceMessage, [&](QStringList args){
		QMessageBox::information(w, "Message", args.join('\n'));
	});

#ifdef USE_EXEC
	return instance.singleExec();
#else
	return a.exec();
#endif
}
