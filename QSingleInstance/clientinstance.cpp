#include "clientinstance.h"
#include <QtEndian>
#include <QCoreApplication>
#include <QDebug>
#include <QDataStream>

ClientInstance::ClientInstance(QLocalSocket *socket, QSingleInstancePrivate *parent) :
	QObject(parent),
	instance(parent),
	socket(socket),
	stream(socket)
{
	connect(socket, &QLocalSocket::readyRead, this, &ClientInstance::newData);
	connect(socket, QOverload<QLocalSocket::LocalSocketError>::of(&QLocalSocket::error),
			this, &ClientInstance::socketError);
	connect(socket, &QLocalSocket::disconnected, this, &ClientInstance::deleteLater);
}

ClientInstance::~ClientInstance()
{
	if(instance->isRunning) {
		if(socket->isOpen())
			socket->close();
		socket->deleteLater();
	}
}

void ClientInstance::newData()
{
	stream.startTransaction();
	QStringList arguments;
	stream >> arguments;

	if(stream.commitTransaction()) {
		socket->write(ACK);
		QTimer::singleShot(5000, socket, &QLocalSocket::disconnectFromServer);
		instance->newArgsReceived(arguments);
	}
}

void ClientInstance::socketError(QLocalSocket::LocalSocketError error)
{
	if(error == QLocalSocket::PeerClosedError)
		return;
	qCWarning(logQSingleInstance) << "Failed to receive arguments from client with error:"
								  << socket->errorString();
	deleteLater();
}

