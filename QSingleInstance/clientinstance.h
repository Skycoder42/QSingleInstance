#ifndef CLIENTINSTANCE_H
#define CLIENTINSTANCE_H

#include <QObject>
#include <QLocalSocket>
#include "qsingleinstance_p.h"

class ClientInstance : public QObject
{
	Q_OBJECT

public:
	explicit ClientInstance(QLocalSocket *socket, QSingleInstancePrivate *parent);
	~ClientInstance();

private slots:
	void newData();
	void socketError(QLocalSocket::LocalSocketError error);

private:
	QSingleInstancePrivate *instance;
	QLocalSocket *socket;

	qint32 argSizeLeft;
	QByteArray argData;
};

#endif // CLIENTINSTANCE_H
