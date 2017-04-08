#include "qsingleinstance_p.h"
#include <QCoreApplication>
#include <QRegularExpression>
#include <QDir>
#include <QtEndian>
#include <QDataStream>
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
	q(q_ptr),
	fullId(QCoreApplication::applicationName()),
	lockName(),
	server(nullptr),
	lockFile(nullptr),
	isMaster(false),
	tryRecover(false),
	isRunning(false),
	startFunc([]()->int{return 0;}),
#ifdef QT_GUI_LIB
	notifyWindow(nullptr),
#endif
#ifdef QT_WIDGETS_LIB
	notifyWidget(nullptr),
#endif
	client(nullptr),
	lockdownTimer(nullptr)
{
#ifdef Q_OS_WIN
	fullId = fullId.toLower();
#endif
	fullId.remove(QRegularExpression(QStringLiteral("[^a-zA-Z0-9_]")));
	fullId.truncate(8);
	fullId.prepend(QStringLiteral("qsingleinstance-"));
	QByteArray hashBase = (QCoreApplication::organizationName() + QLatin1Char('_') + QCoreApplication::applicationName()).toUtf8();
	fullId += QLatin1Char('-') + QString::number(qChecksum(hashBase.data(), hashBase.size()), 16) + QLatin1Char('-');

#ifdef Q_OS_WIN
	DWORD sessID;
	if(ProcessIdToSessionId(GetCurrentProcessId(), &sessID))
		fullId += QString::number(sessID, 16);
#else
	fullId += QString::number(::getuid(), 16);
#endif

	resetLockFile();
}

void QSingleInstancePrivate::resetLockFile()
{
	lockName = QDir::temp().absoluteFilePath(fullId + QStringLiteral("-lockfile"));
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
		bool isListening = server->listen(fullId);

		if (!isListening && server->serverError() == QAbstractSocket::AddressInUseError) {
			if(QLocalServer::removeServer(fullId))
				isListening = server->listen(fullId);
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

	if(recoverAction())
		startFunc();
	else
		qApp->exit(exitCode);
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

	client->connectToServer(fullId, QIODevice::ReadWrite);

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
			qApp->quit();
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
#ifdef QT_GUI_LIB
	if(!notifyWindow.isNull()) {
		notifyWindow->show();
		notifyWindow->raise();
		notifyWindow->alert(0);
		notifyWindow->requestActivate();
	}
#endif
#ifdef QT_WIDGETS_LIB
	if(!notifyWidget.isNull()) {
		notifyWidget->setWindowState(notifyWidget->windowState() & ~ Qt::WindowMinimized);
		notifyWidget->show();
		notifyWidget->raise();
		QApplication::alert(notifyWidget);
		notifyWidget->activateWindow();
	}
#endif
	QMetaObject::invokeMethod(q, "instanceMessage", Qt::QueuedConnection,
							  Q_ARG(QStringList, args));
}

