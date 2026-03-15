#include "ClientHandler.h"
#include <QDateTime>

ClientHandler::ClientHandler(QTcpSocket* socket)
    : socket(socket) {

    connect(socket, &QTcpSocket::readyRead,
        this, &ClientHandler::onReadyRead);
    connect(socket, &QTcpSocket::disconnected,
        this, &ClientHandler::onDisconnected);

    emit logMessage(QString("[%1] 客户端连接：%2:%3")
        .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
        .arg(socket->peerAddress().toString())
        .arg(socket->peerPort()));
}

void ClientHandler::sendPacket(const Packet& packet)
{
    if (!socket) return;
    socket->write(packet.pack());
}

void ClientHandler::onReadyRead() {
    buffer.append(socket->readAll());

    Packet pkt;
    while (Packet::unpack(buffer, pkt)) {
        emit packetArrived(socket, pkt);
    }
}

void ClientHandler::onDisconnected() {
    emit logMessage("客户端断开连接");
    emit disconnected(socket);
}
