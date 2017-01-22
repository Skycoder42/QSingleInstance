#ifndef QSINGLEINSTANCEBASE
#define QSINGLEINSTANCEBASE

#include <QObject>
#include <QLoggingCategory>
#include <functional>
#ifdef QT_GUI_LIB
class QWindow;
#endif
#ifdef QT_WIDGETS_LIB
class QWidget;
#endif

class QSingleInstancePrivate;
//! A class to provide a single instance application
class QSingleInstance : public QObject
{
	Q_OBJECT

	//! Holds the unique ID the instance uses to identify itself and others
	Q_PROPERTY(QString instanceID READ instanceID WRITE setInstanceID NOTIFY instanceIDChanged)
	//! Specifies whether the instance should try to recover the application or not
	Q_PROPERTY(bool autoRecovery READ isAutoRecoveryActive WRITE setAutoRecovery NOTIFY autoRecoveryChanged)
public:
	//! Constructor
	QSingleInstance(QObject *parent = nullptr);

	//! Specifies whether this instance is master or not
	Q_INVOKABLE bool isMaster() const;

	//! READ-Accessor for QSingleInstance::instanceID
	QString instanceID() const;
	//! READ-Accessor for QSingleInstance::autoRecovery
	bool isAutoRecoveryActive() const;

	//! Sets the function to be called when the application starts
	bool setStartupFunction(const std::function<int()> &function);
#ifdef QT_GUI_LIB
	//! Sets a window to be activted on new received arguments
	void setNotifyWindow(QWindow *window);
#endif
#ifdef QT_WIDGETS_LIB
	//! Sets a window to be activted on new received arguments
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
	//! WRITE-Accessor for QSingleInstance::autoRecovery
	void setAutoRecovery(bool autoRecovery);

signals:
	//! Will be emitted if the master receives arguments from another instance
	void instanceMessage(const QStringList &arguments);

	//! NOTIFY-Accessor for QSingleInstance::instanceID
	void instanceIDChanged(QString instanceID);
	//! NOTIFY-Accessor for QSingleInstance::autoRecovery
	void autoRecoveryChanged(bool autoRecovery);

private:
	QSingleInstancePrivate *d_ptr;
	Q_DECLARE_PRIVATE(QSingleInstance)
};

Q_DECLARE_LOGGING_CATEGORY(logQSingleInstance)

#endif // QSINGLEINSTANCEBASE

