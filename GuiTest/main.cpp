#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <qsingleinstance.h>

#define USE_EXEC

int main(int argc, char *argv[])
{
	QGuiApplication app(argc, argv);
	QSingleInstance instance;


#ifdef USE_EXEC
	QQmlApplicationEngine *engine = nullptr;

	instance.setStartupFunction([&]() -> int {
		engine = new QQmlApplicationEngine(NULL);
		engine->rootContext()->setContextProperty("instance", &instance);
		engine->load(QUrl(QStringLiteral("qrc:/main.qml")));

		instance.setNotifyWindow(QGuiApplication::topLevelWindows().first());

		return 0;
	});

	QObject::connect(qApp, &QGuiApplication::aboutToQuit, [&](){
		if(engine)
			delete engine;
	});

	return instance.singleExec();
#else
	if(instance.process()) {
		if(!instance.isMaster())
			return 0;
	} else
		return -1;

	QQmlApplicationEngine engine;
	engine.rootContext()->setContextProperty("instance", &instance);
	engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

	instance.setNotifyWindow(QGuiApplication::topLevelWindows().first());

	return app.exec();
#endif
}

