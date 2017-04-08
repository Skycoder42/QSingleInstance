#include "mainwindow.h"
#include <QApplication>
#include <qsingleinstance.h>

#include <QMessageBox>

#define USE_EXEC

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	QSingleInstance instance;

#ifdef USE_EXEC
	MainWindow *w = nullptr;

	instance.setStartupFunction([&]() -> int {
		w = new MainWindow(nullptr);
		instance.setNotifyWindow(w);
		w->show();
		return 0;
	});

	QObject::connect(qApp, &QApplication::aboutToQuit, [&](){
		if(w)
			delete w;
	});

	QObject::connect(&instance, &QSingleInstance::instanceMessage, [&](QStringList args){
		QMessageBox::information(w, "Message", args.join('\n'));
	});

	return instance.singleExec();
#else
	if(instance.process()) {
		if(!instance.isMaster())
			return 0;
	} else
		return -1;

	MainWindow w;
	instance.setNotifyWindow(&w);

	QObject::connect(&instance, &QSingleInstance::instanceMessage, [&](QStringList args){
		QMessageBox::information(w, "Message", args.join('\n'));
	});

	w.show();
	return a.exec();
#endif
}
