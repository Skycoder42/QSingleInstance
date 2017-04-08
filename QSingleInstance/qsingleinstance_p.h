#ifndef QSINGLEINSTANCEPRIVATE_H
#define QSINGLEINSTANCEPRIVATE_H

#include <QObject>
#include <QLockFile>
#include <QLocalServer>
#include <QLocalSocket>
#include <QScopedPointer>
#include <QTimer>
#ifdef QT_GUI_LIB
#include <QPointer>
#include <QWindow>
#endif
#ifdef QT_WIDGETS_LIB
#include <QPointer>
#include <QWidget>
#endif
#include "qsingleinstance.h"

#define ACK QByteArray("ACK")
#define SPLIT_CHAR QLatin1Char('\n')

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

	bool isRunning;
	std::function<int()> startFunc;
#ifdef QT_GUI_LIB
	QPointer<QWindow> notifyWindow;
#endif
#ifdef QT_WIDGETS_LIB
	QPointer<QWidget> notifyWidget;
#endif

	QLocalSocket *client;
	QTimer *lockdownTimer;

	QSingleInstancePrivate(QSingleInstance *q_ptr);

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
