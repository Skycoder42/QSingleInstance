#include "qsingleinstance.h"
#include "qsingleinstanceprivate.h"
#include <QCoreApplication>
#include <QDir>

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
	Q_D(QSingleInstance);
	d->startInstance();

	if(d->isMaster)
		return true;
	else {
		d->client = new QLocalSocket(this);
		d->client->connectToServer(d->fullId, QIODevice::ReadWrite);
		if(d->client->waitForConnected(5000)) {
			d->clientConnected();
			if(d->client->waitForBytesWritten(5000) && d->client->waitForReadyRead(5000)) {
				if(d->client->read(ACK.size()) == ACK) {
					d->client->disconnect();
					d->client->close();
					return true;
				}
			}
		}

		return d->recoverAction();
	}
}

int QSingleInstance::singleExec()
{
	Q_D(QSingleInstance);
	d->isRunning = true;
	int res = 0;
	d->startInstance();

	if(d->isMaster) {
		d->startFunc();
		res = qApp->exec();
	} else {
		d->sendArgs();
		res = qApp->exec();
	}

	d->isRunning = false;//TODO add stop instance
	return res;
}

bool QSingleInstance::setInstanceID(QString instanceID)
{
	Q_D(QSingleInstance);
	if(d->isRunning)
		return false;
	else if(instanceID != d->fullId){
		d->fullId = instanceID;
		d->lockName = QDir::temp().absoluteFilePath(d->fullId + QStringLiteral("-lockfile"));
		d->lockFile.reset(new QLockFile(d->lockName));
		d->lockFile->setStaleLockTime(0);
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
