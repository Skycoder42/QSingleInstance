#include "clientinstance.h"
#include <QtEndian>
#include <QCoreApplication>
#include <QDebug>

ClientInstance::ClientInstance(QLocalSocket *socket, QSingleInstancePrivate *parent) :
	QObject(parent),
	instance(parent),
	socket(socket),
	argSizeLeft(-1),
	argData()
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
	if(argSizeLeft == -1) {
		if(socket->bytesAvailable() >= (qint64)sizeof(qint32)) {
			qint64 read = socket->read((char*)&(argSizeLeft), sizeof(qint32));
			if(read != sizeof(qint32)) {
				qCWarning(logQSingleInstance) << "Client lost. Client send invalid data";
				deleteLater();
				return;
			}
			argSizeLeft = qFromLittleEndian<qint32>(argSizeLeft);
			argData.reserve(argSizeLeft);
		}
	}
	if(argSizeLeft != -1) {
		argData += socket->readAll();
		if(argData.size() >= argSizeLeft) {
			QStringList arguments = QString::fromUtf8(argData).split(SPLIT_CHAR);
			socket->write(ACK);
			QTimer::singleShot(5000, socket, &QLocalSocket::disconnectFromServer);
			instance->newArgsReceived(arguments);
		}
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

