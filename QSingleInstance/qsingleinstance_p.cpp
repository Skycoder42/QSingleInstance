#include "qsingleinstance_p.h"
#include <QCoreApplication>
#include <QRegularExpression>
#include <QDir>
#include <QtEndian>
#include <QDataStream>
#include <QStandardPaths>
#include "clientinstance.h"

QSingleInstancePrivate::QSingleInstancePrivate(QSingleInstance *q_ptr) :
	QObject(q_ptr),
	q(q_ptr),
	fullId(),
	lockName(),
	server(nullptr),
	lockFile(nullptr),
	isMaster(false),
	tryRecover(false),
	global(false),
	autoClose(false),
	isRunning(false),
	startFunc([]()->int{return 0;}),
	notifyFn(),
	client(nullptr),
	lockdownTimer(nullptr)
{}

QString QSingleInstancePrivate::socketFile() const
{
#ifdef Q_OS_UNIX
	auto socket = fullId + QStringLiteral(".socket");
	if(!global)
		socket = QDir(QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation)).absoluteFilePath(socket);
	return socket;
#else
	return fullId;
#endif
}

void QSingleInstancePrivate::resetLockFile()
{
	lockName = QDir::temp().absoluteFilePath(fullId + QStringLiteral(".lock"));
	lockFile.reset(new QLockFile(lockName));
	lockFile->setStaleLockTime(0);
}

void QSingleInstancePrivate::startInstance()
{
	Q_ASSERT_X(isMaster == lockFile->isLocked(), Q_FUNC_INFO, "Lockfile-state is not as expected");
	if(isMaster)
		return;

	isMaster = lockFile->tryLock();
	if(isMaster) {
		server = new QLocalServer(this);
		if(global)
			server->setSocketOptions(QLocalServer::WorldAccessOption);
		else
			server->setSocketOptions(QLocalServer::UserAccessOption);
		bool isListening = server->listen(socketFile());

		if (!isListening && server->serverError() == QAbstractSocket::AddressInUseError) {
			if(QLocalServer::removeServer(socketFile()))
				isListening = server->listen(socketFile());
		}

		if(!isListening) {
			qCWarning(logQSingleInstance) << "Failed to listen for other instances with error:"
										  << server->errorString();
		} else {
			connect(server, &QLocalServer::newConnection, this, &QSingleInstancePrivate::newConnection);
		}
	}
}

void QSingleInstancePrivate::recover(int exitCode)
{
	if(lockdownTimer) {
		lockdownTimer->stop();
		lockdownTimer->deleteLater();
	}

	if(recoverAction()) {
		auto res = startFunc();
		if(res == EXIT_SUCCESS) {
			if(autoClose)
				connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, q, &QSingleInstance::closeInstance);
		} else if(autoClose)
			q->closeInstance();
	 } else
		QCoreApplication::exit(exitCode);
}

bool QSingleInstancePrivate::recoverAction()
{
	client->deleteLater();
	client = nullptr;

	if(tryRecover) {
		qCInfo(logQSingleInstance) << "Trying to create a new master instance...";
		if(!QFile::exists(lockName) || QFile::remove(lockName)) {
			startInstance();
			if(isMaster) {
				qCInfo(logQSingleInstance) << "Recovery Successful - This instance is now the master.";
				return true;
			}
		}
		qCWarning(logQSingleInstance) << "Failed to create new master instance. The application cannot recover!";
	}
	return false;
}

void QSingleInstancePrivate::sendArgs()
{
	client = new QLocalSocket(this);

	connect(client, &QLocalSocket::connected,
			this, &QSingleInstancePrivate::clientConnected, Qt::QueuedConnection);
	connect(client, &QLocalSocket::readyRead,
			this, &QSingleInstancePrivate::sendResultReady, Qt::QueuedConnection);
	connect(client, SIGNAL(error(QLocalSocket::LocalSocketError)),
			this, SLOT(clientError(QLocalSocket::LocalSocketError)));

	client->connectToServer(socketFile(), QIODevice::ReadWrite);

	lockdownTimer = new QTimer(this);
	lockdownTimer->setInterval(5000);
	lockdownTimer->setSingleShot(true);
	connect(lockdownTimer, &QTimer::timeout, this, [this](){
		qCWarning(logQSingleInstance) << "Master did not respond in time";
		recover(QLocalSocket::SocketTimeoutError);
	});
	lockdownTimer->start();
}

void QSingleInstancePrivate::clientConnected()
{
	lockdownTimer->start();
	performSend(QCoreApplication::arguments());
}

void QSingleInstancePrivate::performSend(const QStringList &arguments)
{
	QDataStream stream(client);
	stream << arguments;
	client->flush();
}

void QSingleInstancePrivate::sendResultReady()
{
	if(client->bytesAvailable() >= ACK.size()) {
		if(client->read(ACK.size()) == ACK) {
			lockdownTimer->stop();
			client->disconnect();
			client->close();
			QCoreApplication::quit();
		} else {
			qCWarning(logQSingleInstance) << "Master didn't ackknowledge the sent arguments";
			recover(QLocalSocket::UnknownSocketError);
		}
	} else
		lockdownTimer->start();
}

void QSingleInstancePrivate::clientError(QLocalSocket::LocalSocketError error)
{
	qCWarning(logQSingleInstance) << "Failed to transfer arguments to master with error:"
								  << client->errorString();
	recover(error);
}

void QSingleInstancePrivate::newConnection()
{
	while(server->hasPendingConnections()) {
		QLocalSocket *instanceClient = server->nextPendingConnection();
		new ClientInstance(instanceClient, this);
	}
}

void QSingleInstancePrivate::newArgsReceived(const QStringList &args)
{
	if(notifyFn)
		notifyFn();
	QMetaObject::invokeMethod(q, "instanceMessage", Qt::QueuedConnection,
							  Q_ARG(QStringList, args));
}

