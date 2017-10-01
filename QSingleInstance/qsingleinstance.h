#ifndef QSINGLEINSTANCEBASE
#define QSINGLEINSTANCEBASE

#include <QObject>
#include <QLoggingCategory>
#include <functional>
#ifdef QT_GUI_LIB
#include <QPointer>
#include <QWindow>
#endif
#ifdef QT_WIDGETS_LIB
#include <QPointer>
#include <QWidget>
#include <QApplication>
#endif

class QSingleInstancePrivate;
//! A class to provide a single instance application
class QSingleInstance : public QObject
{
	Q_OBJECT

	//! Holds the unique ID the instance uses to identify itself and others
	Q_PROPERTY(QString instanceID READ instanceID WRITE setInstanceID RESET resetInstanceID NOTIFY instanceIDChanged)
	//! Specifies whether the instance should try to recover the application or not
	Q_PROPERTY(bool autoRecovery READ isAutoRecoveryActive WRITE setAutoRecovery NOTIFY autoRecoveryChanged)
	//! Specifies whether the instance is local (per user) or global (per machine)
	Q_PROPERTY(bool global READ isGobal WRITE setGlobal)

public:
	//! Constructor
	QSingleInstance(QObject *parent = nullptr);

	//! Specifies whether this instance is master or not
	Q_INVOKABLE bool isMaster() const;

	//! READ-Accessor for QSingleInstance::instanceID
	QString instanceID() const;
	//! READ-Accessor for QSingleInstance::autoRecovery
	bool isAutoRecoveryActive() const;
	//! READ-Accessor for QSingleInstance::global
	bool isGobal() const;

	//! Sets the function to be called when the application starts
	bool setStartupFunction(const std::function<int()> &function);
#ifdef QT_GUI_LIB
	//! Sets a window to be activted on new received arguments
	template <typename T = void>
	void setNotifyWindow(QWindow *window);
#endif
#ifdef QT_WIDGETS_LIB
	//! Sets a window to be activted on new received arguments
	template <typename T = void>
	void setNotifyWindow(QWidget *widget);
#endif

	//! Starts the instance if master or sends arguments to the master
	bool process();
	//! Starts the instance if master or sends custom arguments to the master
	bool process(const QStringList &arguments);

public slots:
	//! Starts the instance if master or sends arguments to the master and runs the Application
	int singleExec(bool autoClose = true);

	//! Stops the singleinstance (but not the application)
	void closeInstance();

	//! WRITE-Accessor for QSingleInstance::instanceID
	bool setInstanceID(QString instanceID);
	//! RESET-Accessor for QSingleInstance::instanceID
	bool resetInstanceID();
	//! WRITE-Accessor for QSingleInstance::autoRecovery
	void setAutoRecovery(bool autoRecovery);
	//! WRITE-Accessor for QSingleInstance::global
	bool setGlobal(bool global, bool recreateId = true);

signals:
	//! Will be emitted if the master receives arguments from another instance
	void instanceMessage(const QStringList &arguments);

	//! NOTIFY-Accessor for QSingleInstance::instanceID
	void instanceIDChanged(QString instanceID);
	//! NOTIFY-Accessor for QSingleInstance::autoRecovery
	void autoRecoveryChanged(bool autoRecovery);

private:
	QSingleInstancePrivate *d;

	void setNotifyWindowFn(const std::function<void()> &fn);
};

Q_DECLARE_LOGGING_CATEGORY(logQSingleInstance)

#ifdef QT_GUI_LIB
template <typename T>
void QSingleInstance::setNotifyWindow(QWindow *window)
{
	QPointer<QWindow> notifyWindow(window);
	setNotifyWindowFn([notifyWindow](){
		if(!notifyWindow.isNull()) {
			notifyWindow->show();
			notifyWindow->raise();
			notifyWindow->alert(0);
			notifyWindow->requestActivate();
		}
	});
}
#endif

#ifdef QT_WIDGETS_LIB
template <typename T>
void QSingleInstance::setNotifyWindow(QWidget *widget)
{
	QPointer<QWidget> notifyWidget(widget);
	setNotifyWindowFn([notifyWidget](){
		if(!notifyWidget.isNull()) {
			notifyWidget->setWindowState(notifyWidget->windowState() & ~ Qt::WindowMinimized);
			notifyWidget->show();
			notifyWidget->raise();
			QApplication::alert(notifyWidget);
			notifyWidget->activateWindow();
		}
	});
}
#endif

#endif // QSINGLEINSTANCEBASE

