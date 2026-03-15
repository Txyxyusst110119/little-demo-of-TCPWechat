#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QWidget>
#include <QTcpServer>
#include <QMap>
#include <QMutex>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include "Packet.h"

class TCPServer : public QWidget {
    Q_OBJECT
public:
    explicit TCPServer(QWidget* parent = nullptr);
    void broadcast(const Packet& packet); // 原有函数
    void broadcast(const Packet& packet, const QString& excludeSender); // 新增重载

private slots:
    void startServer();
    void onNewConnection();
    void onPacketArrived(QTcpSocket* socket, const Packet& packet);
    void onClientDisconnected(QTcpSocket* socket);
    void log(const QString& msg);

private:
    void sendPacket(QTcpSocket* socket, const Packet& packet);

private:
    QTcpServer* server;
    QTextEdit* logEdit;
    QLineEdit* portEdit;
    QPushButton* startBtn;

    QMutex mutex;
    QMap<QTcpSocket*, QString> onlineUsers;
};

#endif
