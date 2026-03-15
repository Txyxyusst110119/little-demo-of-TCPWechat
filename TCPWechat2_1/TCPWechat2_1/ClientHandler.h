#ifndef CLIENTHANDLER_H
#define CLIENTHANDLER_H

#pragma once
#include <QObject>
#include <QTcpSocket>
#include "Packet.h"

class ClientHandler : public QObject {
    Q_OBJECT
public:
    explicit ClientHandler(QTcpSocket* socket);

signals:
    void packetArrived(QTcpSocket* socket, const Packet& packet);
    void disconnected(QTcpSocket* socket);
    void logMessage(const QString& msg);

private slots:
    void onReadyRead();
    void onDisconnected();
    void sendPacket(const Packet& packet);// 线程安全发送

private:
    QTcpSocket* socket;
    QByteArray buffer;   // TCP流缓冲区
};

#endif

