#include <QCoreApplication>
#include <QDebug>
#include <qsingleinstance.h>

#define USE_EXEC

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);
	QSingleInstance instance;
	instance.setGlobal(true); //example for global access

#ifdef USE_EXEC
	instance.setStartupFunction([&]() -> int {
		qDebug() << "Instance running! Start another with -quit as first argument to exit";
		return 0;
	});

	QObject::connect(&instance, &QSingleInstance::instanceMessage, [&](QStringList args){
		qDebug() << "New Instance:" << args;
		if(args.size() > 1 && args[1] == "-quit")
			qApp->quit();
	});

	return instance.singleExec();
#else
	if(instance.process()) {
		if(!instance.isMaster())
			return 0;
	} else
		return -1;

	qDebug() << "Instance running! Start another with -quit as first argument to exit";

	QObject::connect(&instance, &QSingleInstance::instanceMessage, [&](QStringList args){
		qDebug() << "New Instance:" << args;
		if(args.size() > 1 && args[1] == "-quit")
			qApp->quit();
	});

	return a.exec();
#endif
}

