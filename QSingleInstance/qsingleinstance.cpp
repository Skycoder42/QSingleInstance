#include "qsingleinstance.h"
#include "qsingleinstance_p.h"
#include <QCoreApplication>
#include <QDir>
#include <QRegularExpression>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#else
#include <unistd.h>
#include <sys/types.h>
#endif

Q_LOGGING_CATEGORY(logQSingleInstance, "QSingleInstance")

QSingleInstance::QSingleInstance(QObject *parent) :
	QObject(parent),
	d(new QSingleInstancePrivate(this))
{
	resetInstanceID();
}

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

bool QSingleInstance::isGobal() const
{
	return d->global;
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
		d->client->connectToServer(d->socketFile(), QIODevice::ReadWrite);
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
				connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &QSingleInstance::closeInstance);
			res = QCoreApplication::exec();
		} else if(autoClose)
			closeInstance();
	} else {
		d->autoClose = autoClose; //store in case of recovery
		d->sendArgs();
		res = QCoreApplication::exec();
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
	else if(instanceID != d->fullId) {
		d->fullId = instanceID;
		d->resetLockFile();
		emit instanceIDChanged(instanceID);
	}

	return true;
}

bool QSingleInstance::resetInstanceID()
{
	if(d->isRunning || d->isMaster)
		return false;

	d->fullId = QCoreApplication::applicationName();
#ifdef Q_OS_WIN
	d->fullId = d->fullId.toLower();
#endif
	d->fullId.remove(QRegularExpression(QStringLiteral("[^a-zA-Z0-9_]")));
	d->fullId.truncate(8);
	d->fullId.prepend(QStringLiteral("qsingleinstance-"));
	QByteArray hashBase = (QCoreApplication::organizationName() + QLatin1Char('_') + QCoreApplication::applicationName()).toUtf8();
	d->fullId += QLatin1Char('-') + QString::number(qChecksum(hashBase.data(), hashBase.size()), 16);

	if(!d->global) {
		d->fullId += QLatin1Char('-');
#ifdef Q_OS_WIN
		DWORD sessID;
		if(ProcessIdToSessionId(GetCurrentProcessId(), &sessID))
			d->fullId += QString::number(sessID, 16);
#else
		d->fullId += QString::number(::getuid(), 16);
#endif
	}

	d->resetLockFile();
	return true;
}

void QSingleInstance::setAutoRecovery(bool autoRecovery)
{
	if(d->tryRecover != autoRecovery) {
		d->tryRecover = autoRecovery;
		emit autoRecoveryChanged(autoRecovery);
	}
}

bool QSingleInstance::setGlobal(bool global, bool recreateId)
{
	if(d->isRunning || d->isMaster)
		return false;
	else {
		d->global = global;
		if(recreateId)
			return resetInstanceID();
		else
			return true;
	}
}

void QSingleInstance::setNotifyWindowFn(const std::function<void ()> &fn)
{
	d->notifyFn = fn;
}
