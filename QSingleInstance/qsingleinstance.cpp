#include "qsingleinstance.h"
#include "qsingleinstance_p.h"
#include <QCoreApplication>
#include <QDir>

Q_LOGGING_CATEGORY(logQSingleInstance, "QSingleInstance")

QSingleInstance::QSingleInstance(QObject *parent) :
	QObject(parent),
	d(new QSingleInstancePrivate(this))
{}

QString QSingleInstance::instanceID() const
{
	return d->fullId;
}

bool QSingleInstance::isMaster() const
{
	return d->isMaster;
}

bool QSingleInstance::isAutoRecoveryActive() const
{
	return d->tryRecover;
}

bool QSingleInstance::setStartupFunction(const std::function<int()> &function)
{
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
	d->notifyWindow = window;
#ifdef QT_WIDGETS_LIB
	d->notifyWidget = nullptr;
#endif
}
#endif

#ifdef QT_WIDGETS_LIB
void QSingleInstance::setNotifyWindow(QWidget *widget)
{
	d->notifyWidget = widget;
#ifdef QT_GUI_LIB
	d->notifyWindow = nullptr;
#endif
}
#endif

bool QSingleInstance::process()
{
	return process(QCoreApplication::arguments());
}

bool QSingleInstance::process(const QStringList &arguments)
{
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
	Q_ASSERT_X(!d->isRunning, Q_FUNC_INFO, "Nested call detected!");
	d->isRunning = true;
	int res = 0;
	d->startInstance();

	if(d->isMaster) {
		res = d->startFunc();
		if(res == EXIT_SUCCESS) {
			if(autoClose)
				connect(qApp, &QCoreApplication::aboutToQuit, this, &QSingleInstance::closeInstance);
			res = qApp->exec();
		} else if(autoClose)
			closeInstance();
	} else {
		d->autoClose = autoClose; //store in case of recovery
		d->sendArgs();
		res = qApp->exec();
		d->autoClose = false; //and reset
	}

	d->isRunning = false;
	return res;
}

void QSingleInstance::closeInstance()
{
	if(d->isMaster) {
		d->server->close();
		d->server->deleteLater();
		d->server = nullptr;
		d->lockFile->unlock();
		d->isMaster = false;
	}
}

bool QSingleInstance::setInstanceID(QString instanceID)
{
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
	if(d->tryRecover != autoRecovery) {
		d->tryRecover = autoRecovery;
		emit autoRecoveryChanged(autoRecovery);
	}
}
