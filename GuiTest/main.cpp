#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <qsingleinstance.h>

#define USE_EXEC

int main(int argc, char *argv[])
{
	QGuiApplication app(argc, argv);

	QSingleInstance instance;
	QQmlApplicationEngine *engine;
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
		engine = new QQmlApplicationEngine(NULL);
		engine->rootContext()->setContextProperty("instance", &instance);
		engine->load(QUrl(QStringLiteral("qrc:/main.qml")));
#ifdef USE_EXEC
		return 0;
	});
#endif

	QObject::connect(qApp, &QGuiApplication::aboutToQuit, [&](){
		delete engine;
	});

#ifdef USE_EXEC
	return instance.singleExec();
#else
	return app.exec();
#endif
}

