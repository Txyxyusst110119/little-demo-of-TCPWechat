#include "TCPServer.h"
#include "ClientHandler.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QThread>
#include <QDateTime>
#include <QMutexLocker>

TCPServer::TCPServer(QWidget* parent) : QWidget(parent) {
    server = new QTcpServer(this);

    portEdit = new QLineEdit("8888");
    startBtn = new QPushButton("启动服务器");
    logEdit = new QTextEdit;
    logEdit->setReadOnly(true);

    QHBoxLayout* top = new QHBoxLayout;
    top->addWidget(portEdit);
    top->addWidget(startBtn);

    QVBoxLayout* main = new QVBoxLayout(this);
    main->addLayout(top);
    main->addWidget(logEdit);

    connect(startBtn, &QPushButton::clicked, this, &TCPServer::startServer);
    connect(server, &QTcpServer::newConnection, this, &TCPServer::onNewConnection);
}

void TCPServer::startServer() {
    quint16 port = portEdit->text().toUShort();
    if (!server->listen(QHostAddress::Any, port)) {
        QMessageBox::critical(this, "错误", server->errorString());
        return;
    }
    log(QString("服务器启动，监听端口 %1").arg(port));
}

void TCPServer::onNewConnection() {
    while (server->hasPendingConnections()) {
        QTcpSocket* socket = server->nextPendingConnection();

        QThread* thread = new QThread(this);
        ClientHandler* handler = new ClientHandler(socket);

        socket->moveToThread(thread);
        handler->moveToThread(thread);

        connect(thread, &QThread::finished, handler, &QObject::deleteLater);
        connect(thread, &QThread::finished, thread, &QObject::deleteLater);

        connect(handler, &ClientHandler::packetArrived, this, &TCPServer::onPacketArrived);
        connect(handler, &ClientHandler::disconnected, this, &TCPServer::onClientDisconnected);
        connect(handler, &ClientHandler::logMessage, this, &TCPServer::log);

        thread->start();
    }
}

void TCPServer::onPacketArrived(QTcpSocket* socket, const Packet& pkt) {
    QMutexLocker locker(&mutex);

    if (pkt.type == LOGIN_REQUEST) {
        QString username = QString::fromUtf8(pkt.data);
        if (onlineUsers.values().contains(username)) {
            Packet resp{ LOGIN_RESPONSE, "fail:用户名已存在" };
            sendPacket(socket, resp);
            return;
        }

        onlineUsers[socket] = username;
        Packet ok{ LOGIN_RESPONSE, "success" };
        sendPacket(socket, ok);

        QStringList userList;
        for (const QString& user : onlineUsers.values()) {
            if (user != username) {
                userList.append(user);
            }
        }
        Packet listPacket{ USER_LIST, userList.join(",").toUtf8() };
        sendPacket(socket, listPacket);

        Packet online{ USER_ONLINE, username.toUtf8() };
        for (QTcpSocket* s : onlineUsers.keys()) {
            if (s != socket) {
                sendPacket(s, online);
            }
        }

        log(QString("用户上线：%1").arg(username));
    }
    else if (pkt.type == CHAT_MESSAGE) {
        broadcast(pkt);
    }
    else if (pkt.type == PRIVATE_CHAT_REQUEST) {
        QString data = QString::fromUtf8(pkt.data);
        QStringList parts = data.split("|");
        if (parts.size() >= 2) {
            QString requester = parts[0];
            QString targetUser = parts[1];

            QTcpSocket* targetSocket = nullptr;
            for (auto it = onlineUsers.begin(); it != onlineUsers.end(); ++it) {
                if (it.value() == targetUser) {
                    targetSocket = it.key();
                    break;
                }
            }

            if (targetSocket && targetSocket->isWritable()) {
                sendPacket(targetSocket, pkt);
                log(QString("转发私聊请求：%1 -> %2").arg(requester).arg(targetUser));
            }
            else {
                log(QString("私聊请求转发失败：%2 不在线").arg(targetUser));
            }
        }
    }
    else if (pkt.type == PRIVATE_CHAT_RESPONSE) {
        QString data = QString::fromUtf8(pkt.data);
        QStringList parts = data.split("|");
        if (parts.size() >= 3) {
            QString originalReceiver = parts[0];
            QString originalRequester = parts[1];
            QString result = parts[2];

            QTcpSocket* targetSocket = nullptr;
            for (auto it = onlineUsers.begin(); it != onlineUsers.end(); ++it) {
                if (it.value() == originalRequester) {
                    targetSocket = it.key();
                    break;
                }
            }

            if (targetSocket && targetSocket->isWritable()) {
                sendPacket(targetSocket, pkt);
                log(QString("转发私聊响应：%1 -> %2，结果：%3")
                    .arg(originalReceiver).arg(originalRequester).arg(result));
            }
            else {
                log(QString("私聊响应转发失败：原发起者%1不在线").arg(originalRequester));
            }
        }
    }
    else if (pkt.type == PRIVATE_CHAT_MESSAGE) {
        QString data = QString::fromUtf8(pkt.data);
        QStringList parts = data.split("|");

        if (parts.size() < 3) {
            log("私聊消息格式错误");
            return;
        }

        QString sender = parts[0];
        QString receiver = parts[1];
        QString content = parts.mid(2).join("|");

        QTcpSocket* targetSocket = nullptr;
        for (auto it = onlineUsers.begin(); it != onlineUsers.end(); ++it) {
            if (it.value() == receiver) {
                targetSocket = it.key();
                break;
            }
        }

        if (targetSocket && targetSocket->isWritable()) {
            Packet privatePacket;
            privatePacket.type = PRIVATE_CHAT_MESSAGE;
            privatePacket.data = pkt.data;
            sendPacket(targetSocket, privatePacket);

            log(QString("私聊消息：%1 -> %2: %3").arg(sender).arg(receiver).arg(content));
        }
    }


    else if (pkt.type == IMAGE_TRANSFER) {
        QByteArray allData = pkt.data;
        // ========== 核心修复：按新格式解析（4字节头部长度 + 头部 + 二进制） ==========
        // 1. 先校验数据包长度（至少4字节头部长度）
        if (allData.size() < 4) {
            log("图片数据包过短（不足4字节头部长度）");
            return;
        }

        // 2. 解析头部长度（网络字节序）
        QDataStream lenStream(allData.left(4));
        lenStream.setByteOrder(QDataStream::BigEndian);
        qint32 headerLen;
        lenStream >> headerLen;

        // 3. 校验头部长度合法性（避免恶意数据）
        if (headerLen <= 0 || headerLen >= allData.size() - 4) {
            log(QString("图片头部长度非法（长度：%1，总数据：%2字节）").arg(headerLen).arg(allData.size()));
            return;
        }

        // 4. 拆分头部文本和二进制数据
        QByteArray headerData = allData.mid(4, headerLen); // 4字节后取headerLen字节作为头部
        QString headerStr = QString::fromUtf8(headerData);
        QStringList headerParts = headerStr.split("|");

        if (headerParts.isEmpty()) {
            log("图片头部为空");
            return;
        }

        QString typeFlag = headerParts[0];
        QString sender = headerParts.size() >= 2 ? headerParts[1] : "";

        // 5. 群聊图片：广播（排除发送者）
        if (typeFlag == "GROUP") {
            if (headerParts.size() != 4) {
                log(QString("群聊图片头部格式错误（预期4段，实际%1段）").arg(headerParts.size()));
                return;
            }
            broadcast(pkt, sender); // 调用重载的广播函数，排除发送者
            log(QString("转发群聊图片：%1（文件名：%2）").arg(sender).arg(headerParts[2]));
        }
        // 6. 私聊图片：转发给指定接收者
        else if (typeFlag == "PRIVATE") {
            if (headerParts.size() != 5) {
                log(QString("私聊图片头部格式错误（预期5段，实际%1段）").arg(headerParts.size()));
                return;
            }
            QString receiver = headerParts[2];
            // 查找接收者的Socket
            QTcpSocket* targetSocket = nullptr;
            for (auto it = onlineUsers.begin(); it != onlineUsers.end(); ++it) {
                if (it.value() == receiver) {
                    targetSocket = it.key();
                    break;
                }
            }
            if (targetSocket && targetSocket->isWritable()) {
                sendPacket(targetSocket, pkt);
                log(QString("转发私聊图片：%1 -> %2（文件名：%3）").arg(sender).arg(receiver).arg(headerParts[3]));
            }
            else {
                log(QString("私聊图片转发失败：接收者%1不在线").arg(receiver));
            }
        }
        else {
            log(QString("未知图片类型：%1").arg(typeFlag));
        }
    }
}

// 修复：实现重载的 broadcast 函数（放在 onPacketArrived 外）
// 重载的广播函数（排除发送者）
void TCPServer::broadcast(const Packet& packet, const QString& excludeSender) {
    QByteArray bytes = packet.pack();
    for (auto it = onlineUsers.begin(); it != onlineUsers.end(); ++it) {
        // 排除发送者，避免自己收到自己的群聊图片
        if (it.value() != excludeSender) {
            it.key()->write(bytes);
        }
    }
}



// 原有广播函数（兼容无排除的场景）
void TCPServer::broadcast(const Packet& packet) {
    broadcast(packet, "");
}

void TCPServer::onClientDisconnected(QTcpSocket* socket) {
    QMutexLocker locker(&mutex);
    if (onlineUsers.contains(socket)) {
        QString name = onlineUsers.take(socket);
        Packet offline{ USER_OFFLINE, name.toUtf8() };
        for (QTcpSocket* s : onlineUsers.keys()) {
            sendPacket(s, offline);
        }
        log(QString("用户下线：%1").arg(name));
    }
    socket->deleteLater();
}

void TCPServer::sendPacket(QTcpSocket* socket, const Packet& packet) {
    socket->write(packet.pack());
}

void TCPServer::log(const QString& msg) {
    logEdit->append(QString("[%1] %2")
        .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
        .arg(msg));
}