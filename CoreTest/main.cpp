#include <QCoreApplication>
#include <QDebug>
#include <qsingleinstance.h>

#define USE_EXEC

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);

	QSingleInstance instance;

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
		qDebug() << "Instance running! Start another with -quit as first argument to exit";
#ifdef USE_EXEC
		return 0;
	});
#endif

	QObject::connect(&instance, &QSingleInstance::instanceMessage, [&](QStringList args){
		qDebug() << "New Instance:" << args;
		if(args.size() > 1 && args[1] == "-quit")
			qApp->quit();
	});

#ifdef USE_EXEC
	return instance.singleExec();
#else
	return a.exec();
#endif
}

