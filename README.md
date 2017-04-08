# QSingleInstance
A simple single instance application for Qt

## Features
- Allows to run an application as "single instance"
	- This means only one instance at a time of the application will be running
	- Subsequent starts send their arguments to the running instance, instead of starting a new one
	- Instances are generated per user
- It supports all kinds of applications, with or without gui.
	- For gui/widgets applications, you can register a window to be activated

## Installation
The package is providet as qpm package, [`de.skycoder42.qsingleinstance`](https://www.qpm.io/packages/de.skycoder42.qsingleinstance/index.html). To install:

1. Install qpm (See [GitHub - Installing](https://github.com/Cutehacks/qpm/blob/master/README.md#installing))
2. In your projects root directory, run `qpm install de.skycoder42.qsingleinstance`
3. Include qpm to your project by adding `include(vendor/vendor.pri)` to your `.pro` file

Check their [GitHub - Usage for App Developers](https://github.com/Cutehacks/qpm/blob/master/README.md#usage-for-app-developers) to learn more about qpm.

## Usage
Check the three example projects to see what you can do for each application type. The following shows a simple console-based example:

```cpp
#include <qsingleinstance.h>

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);
	QSingleInstance instance;

	//this lambda only gets executed on the first instance
	instance.setStartupFunction([&]() -> int {
		qDebug() << "Instance running! Start another with -quit as first argument to exit";
		return 0;
	});

	//new instances send their arguments and this signal is received on the main instance
	QObject::connect(&instance, &QSingleInstance::instanceMessage, [&](QStringList args){
		qDebug() << "New Instance:" << args;
		if(args.size() > 1 && args[1] == "-quit")
			qApp->quit();
	});

	return instance.singleExec();
}

```