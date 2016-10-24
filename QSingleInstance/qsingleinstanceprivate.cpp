#include "qsingleinstanceprivate.h"
#include <QCoreApplication>
#include <QRegularExpression>
#include <QDir>
#include <QtEndian>
#ifdef QT_WIDGETS_LIB
#include <QApplication>
#endif
#include "clientinstance.h"

#ifdef Q_OS_WIN
#include <qt_windows.h>
#else
#include <unistd.h>
#include <sys/types.h>
#endif

QSingleInstancePrivate::QSingleInstancePrivate(QSingleInstance *q_ptr) :
	QObject(q_ptr),
	q_ptr(q_ptr),
	fullId(QCoreApplication::applicationName()),
	lockName(),
	server(NULL),
	lockFile(NULL),
	isMaster(false),
	tryRecover(false),
	isRunning(false),
	startFunc([]()->int{return 0;}),
#ifdef QT_GUI_LIB
	notifyWindow(NULL),
#endif
#ifdef QT_WIDGETS_LIB
	notifyWidget(NULL),
#endif
	client(NULL),
	lockdownTimer(NULL)
{
#ifdef Q_OS_WIN
	this->fullId = this->fullId.toLower();
#endif
	this->fullId.remove(QRegularExpression(QStringLiteral("[^a-zA-Z0-9_]")));
	this->fullId.truncate(8);
	this->fullId.prepend(QStringLiteral("qsingleinstance-"));
	QByteArray hashBase = (QCoreApplication::organizationName() + QLatin1Char('_') + QCoreApplication::applicationName()).toUtf8();
	this->fullId += QLatin1Char('-') + QString::number(qChecksum(hashBase.data(), hashBase.size()), 16) + QLatin1Char('-');

#ifdef Q_OS_WIN
	DWORD sessID;
	if(ProcessIdToSessionId(GetCurrentProcessId(), &sessID))
		this->fullId += QString::number(sessID, 16);
#else
	this->fullId += QString::number(::getuid(), 16);
#endif

	this->resetLockFile();
}

void QSingleInstancePrivate::resetLockFile()
{
	this->lockName = QDir::temp().absoluteFilePath(this->fullId + QStringLiteral("-lockfile"));
	this->lockFile.reset(new QLockFile(this->lockName));
	this->lockFile->setStaleLockTime(0);
}

void QSingleInstancePrivate::startInstance()
{
	Q_ASSERT_X(this->isMaster == this->lockFile->isLocked(), Q_FUNC_INFO, "Lockfile-state is not as expected");
	if(this->isMaster)
		return;

	this->isMaster = this->lockFile->tryLock();
	if(this->isMaster) {
		this->server = new QLocalServer(this);
		bool isListening = this->server->listen(this->fullId);

		if (!isListening && this->server->serverError() == QAbstractSocket::AddressInUseError) {
			if(QLocalServer::removeServer(this->fullId))
				isListening = this->server->listen(this->fullId);
		}

		if(!isListening) {
			qpCWarning(logQSingleInstance, QStringLiteral("Failed to listen for other instances with error \"%1\"")
					   .arg(this->server->errorString()));
		} else {
			connect(this->server, &QLocalServer::newConnection, this, &QSingleInstancePrivate::newConnection);
		}
	}
}

void QSingleInstancePrivate::recover(int exitCode)
{
	if(this->lockdownTimer) {
		this->lockdownTimer->stop();
		this->lockdownTimer->deleteLater();
	}

	if(this->recoverAction())
		this->startFunc();
	else
		qApp->exit(exitCode);
}

bool QSingleInstancePrivate::recoverAction()
{
	this->client->deleteLater();
	this->client = NULL;

	if(this->tryRecover) {
		qCInfo(logQSingleInstance, "Trying to create a new master instance...");
		if(!QFile::exists(this->lockName) || QFile::remove(this->lockName)) {
			this->startInstance();
			if(this->isMaster) {
				qCInfo(logQSingleInstance, "Recovery Successful - This instance is now the master.");
				return true;
			}
		}
		qCWarning(logQSingleInstance, "Failed to create new master instance. The application cannot recover!");
	}
	return false;
}

void QSingleInstancePrivate::sendArgs()
{
	this->client = new QLocalSocket(this);

	connect(this->client, &QLocalSocket::connected,
			this, &QSingleInstancePrivate::clientConnected, Qt::QueuedConnection);
	connect(this->client, &QLocalSocket::readyRead,
			this, &QSingleInstancePrivate::sendResultReady, Qt::QueuedConnection);
	connect(this->client, SIGNAL(error(QLocalSocket::LocalSocketError)),
			this, SLOT(clientError(QLocalSocket::LocalSocketError)));

	this->client->connectToServer(this->fullId, QIODevice::ReadWrite);

	this->lockdownTimer = new QTimer(this);
	this->lockdownTimer->setInterval(5000);
	this->lockdownTimer->setSingleShot(true);
	connect(this->lockdownTimer, &QTimer::timeout, this, [this](){
		qCWarning(logQSingleInstance, "Master did not respond in time");
		this->recover(QLocalSocket::SocketTimeoutError);
	});
	this->lockdownTimer->start();
}

void QSingleInstancePrivate::clientConnected()
{
	this->lockdownTimer->start();
	this->performSend(QCoreApplication::arguments());
}

void QSingleInstancePrivate::performSend(const QStringList &arguments)
{
	QByteArray sendData = arguments.join(SPLIT_CHAR).toUtf8();
	qint32 sendSize = qToLittleEndian<qint32>(sendData.size());
	this->client->write((char*)&sendSize, sizeof(qint32));
	this->client->write(sendData);
}

void QSingleInstancePrivate::sendResultReady()
{
	if(this->client->bytesAvailable() >= ACK.size()) {
		if(this->client->read(ACK.size()) == ACK) {
			this->lockdownTimer->stop();
			this->client->disconnect();
			this->client->close();
			qApp->quit();
		} else {
			qCWarning(logQSingleInstance, "Master didn't ackknowledge the sent arguments");
			this->recover(QLocalSocket::UnknownSocketError);
		}
	} else
		this->lockdownTimer->start();
}

void QSingleInstancePrivate::clientError(QLocalSocket::LocalSocketError error)
{
	qpCWarning(logQSingleInstance, QStringLiteral("Failed to transfer arguments to master with error \"%1\"")
			   .arg(this->client->errorString()));
	this->recover(error);
}

void QSingleInstancePrivate::newConnection()
{
	while(this->server->hasPendingConnections()) {
		QLocalSocket *instanceClient = this->server->nextPendingConnection();
		new ClientInstance(instanceClient, this);//Reffered by QObject
	}
}

void QSingleInstancePrivate::newArgsReceived(const QStringList &args)
{
	Q_Q(QSingleInstance);
#ifdef QT_GUI_LIB
	if(!this->notifyWindow.isNull()) {
		this->notifyWindow->show();
		this->notifyWindow->raise();
		this->notifyWindow->alert(0);
		this->notifyWindow->requestActivate();
	}
#endif
#ifdef QT_WIDGETS_LIB
	if(!this->notifyWidget.isNull()) {
		this->notifyWidget->setWindowState(this->notifyWidget->windowState() & ~ Qt::WindowMinimized);
		this->notifyWidget->show();
		this->notifyWidget->raise();
		QApplication::alert(this->notifyWidget);
		this->notifyWidget->activateWindow();
	}
#endif
	emit q->instanceMessage(args);
}

