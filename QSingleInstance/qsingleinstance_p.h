#ifndef QSINGLEINSTANCEPRIVATE_H
#define QSINGLEINSTANCEPRIVATE_H

#include <QObject>
#include <QLockFile>
#include <QLocalServer>
#include <QLocalSocket>
#include <QScopedPointer>
#include <QTimer>
#include "qsingleinstance.h"

#define ACK QByteArray("ACK")

class QSingleInstancePrivate : public QObject
{
	Q_OBJECT

public:
	QSingleInstance *q;

	QString fullId;
	QString lockName;
	QLocalServer *server;
	QScopedPointer<QLockFile> lockFile;
	bool isMaster;
	bool tryRecover;
	bool global;
	bool autoClose;

	bool isRunning;
	std::function<int()> startFunc;
	std::function<void()> notifyFn;

	QLocalSocket *client;
	QTimer *lockdownTimer;

	QSingleInstancePrivate(QSingleInstance *q_ptr);

	QString socketFile() const;
	void resetLockFile();

public slots:
	void startInstance();
	void recover(int exitCode);
	bool recoverAction();

	void sendArgs();
	void clientConnected();
	void performSend(const QStringList &arguments);
	void sendResultReady();
	void clientError(QLocalSocket::LocalSocketError error);

	void newConnection();

	void newArgsReceived(const QStringList &args);
};

#endif // QSINGLEINSTANCEPRIVATE_H
