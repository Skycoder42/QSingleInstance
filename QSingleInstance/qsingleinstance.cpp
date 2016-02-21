#include "qsingleinstance.h"
#include "qsingleinstanceprivate.h"
#include <QCoreApplication>
#include <QDir>

Q_LOGGING_CATEGORY(logQSingleInstance, "QSingleInstance")

QSingleInstance::QSingleInstance(QObject *parent) :
	QObject(parent),
	d_ptr(new QSingleInstancePrivate(this))
{}

QString QSingleInstance::instanceID() const
{
	const Q_D(QSingleInstance);
	return d->fullId;
}

bool QSingleInstance::isMaster() const
{
	const Q_D(QSingleInstance);
	return d->isMaster;
}

bool QSingleInstance::isAutoRecoveryActive() const
{
	const Q_D(QSingleInstance);
	return d->tryRecover;
}

bool QSingleInstance::setStartupFunction(const std::function<int()> &function)
{
	Q_D(QSingleInstance);
	if(d->isRunning)
		return false;
	else {
		d->startFunc = function;
		return true;
	}
}

#ifdef QT_GUI_LIB
void QSingleInstance::setNotifyWindow(QWindow *window)
{
	Q_D(QSingleInstance);
	d->notifyWindow = window;
#ifdef QT_WIDGETS_LIB
	d->notifyWidget = NULL;
#endif
}
#endif

#ifdef QT_WIDGETS_LIB
void QSingleInstance::setNotifyWindow(QWidget *widget)
{
	Q_D(QSingleInstance);
	d->notifyWidget = widget;
#ifdef QT_GUI_LIB
	d->notifyWindow = NULL;
#endif
}
#endif

bool QSingleInstance::process()
{
	return this->process(QCoreApplication::arguments());
}

bool QSingleInstance::process(const QStringList &arguments)
{
	Q_D(QSingleInstance);
	d->startInstance();

	if(d->isMaster)
		return true;
	else if(!arguments.isEmpty()) {
		d->client = new QLocalSocket(this);
		d->client->connectToServer(d->fullId, QIODevice::ReadWrite);
		if(d->client->waitForConnected(5000)) {
			d->performSend(arguments);
			if(d->client->waitForBytesWritten(5000) && d->client->waitForReadyRead(5000)) {
				if(d->client->read(ACK.size()) == ACK) {
					d->client->disconnect();
					d->client->close();
					return true;
				}
			}
		}

		return d->recoverAction();
	} else
		return true;
}

int QSingleInstance::singleExec(bool autoClose)
{
	Q_D(QSingleInstance);
	Q_ASSERT_X(!d->isRunning, Q_FUNC_INFO, "Nested call detected!");
	d->isRunning = true;
	int res = 0;
	d->startInstance();

	if(d->isMaster) {
		if(autoClose)
			connect(qApp, &QCoreApplication::aboutToQuit, this, &QSingleInstance::closeInstance);
		d->startFunc();
		res = qApp->exec();
	} else {
		d->sendArgs();
		res = qApp->exec();
	}

	d->isRunning = false;
	return res;
}

void QSingleInstance::closeInstance()
{
	Q_D(QSingleInstance);
	if(d->isMaster) {
		d->server->close();
		d->server->deleteLater();
		d->server = NULL;
		d->lockFile->unlock();
		d->isMaster = false;
	}
}

bool QSingleInstance::setInstanceID(QString instanceID)
{
	Q_D(QSingleInstance);
	if(d->isRunning || d->isMaster)
		return false;
	else if(instanceID != d->fullId){
		d->fullId = instanceID;
		d->resetLockFile();
		emit instanceIDChanged(instanceID);
	}

	return true;
}

void QSingleInstance::setAutoRecovery(bool autoRecovery)
{
	Q_D(QSingleInstance);
	if(d->tryRecover != autoRecovery) {
		d->tryRecover = autoRecovery;
		emit autoRecoveryChanged(autoRecovery);
	}
}
