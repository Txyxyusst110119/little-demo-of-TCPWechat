#include "TCPClient.h"
#include <QTextCursor>
#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QMenu>       
#include <QAction> 
#include <QFileDialog>   // 用于文件选择对话框（getOpenFileName）
#include <QFile>         // 用于文件读写
#include <QFileInfo>     // 用于获取文件名/文件信息
#include <QDir>          // 用于获取用户主目录（QDir::homePath()）
#include <QMessageBox>   // 用于弹窗提示（已引入，但确认保留）
#include <QByteArray> 

// 构造函数：初始化界面
TCPClient::TCPClient(QWidget* parent) : QWidget(parent) {
    // 初始化Socket
    tcpSocket = new QTcpSocket(this);
    tcpSocket->reset(); // 初始重置

    // 连接Socket信号
    connect(tcpSocket, &QTcpSocket::connected, this, &TCPClient::onConnected);
    connect(tcpSocket, &QTcpSocket::disconnected, this, &TCPClient::onDisconnected);
    connect(tcpSocket, &QTcpSocket::readyRead, this, &TCPClient::onReadyRead);
    connect(tcpSocket, &QTcpSocket::errorOccurred, this, &TCPClient::onSocketError);

    // 初始化界面
    initLoginUI();
    initChatUI();

    // 主布局：默认显示登录界面
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(loginWidget);
    mainLayout->addWidget(chatWidget);
    chatWidget->hide(); // 初始隐藏聊天界面

    // 窗口设置
    setWindowTitle("TCP聊天客户端(Qt + C++)");
    resize(800, 600);
    setStyleSheet("QWidget { font-size: 14px; }");
}

// 初始化登录界面
void TCPClient::initLoginUI() {
    loginWidget = new QWidget(this);
    QVBoxLayout* loginLayout = new QVBoxLayout(loginWidget);
    loginLayout->setSpacing(20);
    loginLayout->setContentsMargins(50, 50, 50, 50);

    // IP输入行
    QHBoxLayout* ipLayout = new QHBoxLayout();
    QLabel* ipLabel = new QLabel("服务器IP：");
    ipEdit = new QLineEdit("127.0.0.1"); // 默认本地IP
    ipLayout->addWidget(ipLabel);
    ipLayout->addWidget(ipEdit);

    // 端口输入行
    QHBoxLayout* portLayout = new QHBoxLayout();
    QLabel* portLabel = new QLabel("服务器端口：");
    portEdit = new QLineEdit("8888"); // 默认端口
    portLayout->addWidget(portLabel);
    portLayout->addWidget(portEdit);

    // 用户名输入行
    QHBoxLayout* usernameLayout = new QHBoxLayout();
    QLabel* usernameLabel = new QLabel("用户名：");
    usernameEdit = new QLineEdit();
    usernameLayout->addWidget(usernameLabel);
    usernameLayout->addWidget(usernameEdit);

    // 登录按钮
    loginBtn = new QPushButton("登录");
    loginBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #2196F3;
            color: white;
            border: none;
            border-radius: 6px;
            padding: 10px;
        }
        QPushButton:hover {
            background-color: #0b7dda;
        }
    )");
    connect(loginBtn, &QPushButton::clicked, this, &TCPClient::onLoginClicked);

    // 添加到登录布局
    loginLayout->addLayout(ipLayout);
    loginLayout->addLayout(portLayout);
    loginLayout->addLayout(usernameLayout);
    loginLayout->addWidget(loginBtn, 0, Qt::AlignCenter);
}




// 初始化聊天界面
void TCPClient::initChatUI() {
    chatWidget = new QWidget(this);
    QHBoxLayout* chatMainLayout = new QHBoxLayout(chatWidget);  // 改为水平布局

    // 左侧：聊天区域
    QWidget* chatArea = new QWidget();
    QVBoxLayout* chatLayout = new QVBoxLayout(chatArea);
    chatLayout->setSpacing(10);
    chatLayout->setContentsMargins(0, 0, 0, 0);

    // 添加聊天模式标签（在聊天区域顶部）
    QLabel* chatModeLabel = new QLabel("群聊模式");
    chatModeLabel->setStyleSheet("color: gray; font-size: 12px; padding: 5px;");
    chatModeLabel->setAlignment(Qt::AlignCenter);
    chatLayout->addWidget(chatModeLabel);

    // 聊天日志框
    chatLogEdit = new QTextEdit();
    chatLogEdit->setReadOnly(true);
    chatLogEdit->setStyleSheet(R"(
        QTextEdit {
            background: #f0f0f0;
            border: 1px solid #ddd;
            border-radius: 8px;
            padding: 10px;
            font-size: 14px;
        }
    )");

    // 消息输入行
    QHBoxLayout* msgInputLayout = new QHBoxLayout();
    msgInputEdit = new QLineEdit();
    msgInputEdit->setPlaceholderText("输入聊天消息...");
    sendMsgBtn = new QPushButton("发送");
    sendMsgBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #4CAF50;
            color: white;
            border: none;
            border-radius: 6px;
            padding: 8px 20px;
        }
        QPushButton:hover {
            background-color: #45a049;
        }
    )");
    connect(sendMsgBtn, &QPushButton::clicked, this, &TCPClient::onSendMsgClicked);

    // 退出按钮
    logoutBtn = new QPushButton("退出登录");
    logoutBtn->setStyleSheet(R"(
    QPushButton {
        background-color: #f44336;
        color: white;
        border: none;
        border-radius: 6px;
        padding: 8px 20px;
    }
    QPushButton:hover {
        background-color: #d32f2f;
    }
)");
    connect(logoutBtn, &QPushButton::clicked, [this]() {
        if (tcpSocket->state() != QTcpSocket::UnconnectedState) {
            tcpSocket->abort();
        }

        // 关闭所有私聊窗口
        QList<QString> keys = privateChatWindows.keys();
        foreach(const QString & key, keys) {
            closePrivateChatWindow(key);
        }

        username.clear();
        usernameEdit->clear();
        ipEdit->clear();
        portEdit->clear();
        chatWidget->hide();
        loginWidget->show();
        chatLogEdit->clear();
        msgInputEdit->clear();
        userListWidget->clear();  // 清空用户列表
        tcpSocket->reset();
        });

    // 按钮布局
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addWidget(sendMsgBtn);

    QPushButton* managePrivateChatsBtn = new QPushButton("📁 私聊窗口");
    managePrivateChatsBtn->setStyleSheet(R"(
    QPushButton {
        background-color: #9C27B0;
        color: white;
        border: none;
        border-radius: 6px;
        padding: 8px 20px;
    }
    QPushButton:hover {
        background-color: #7B1FA2;
    }
)");
    connect(managePrivateChatsBtn, &QPushButton::clicked, [this]() {
        showPrivateChatWindowManager();
        });

    btnLayout->addWidget(managePrivateChatsBtn);

    btnLayout->addWidget(logoutBtn);

    msgInputLayout->addWidget(msgInputEdit);
    msgInputLayout->addLayout(btnLayout);

    chatLayout->addWidget(chatLogEdit);
    chatLayout->addLayout(msgInputLayout);

    // 群聊窗口添加“发送图片”
    QPushButton* sendGroupImageBtn = new QPushButton("发送图片");
    sendGroupImageBtn->setStyleSheet(
        "QPushButton {"
        "   background-color: #FF9800;"
        "   color: white;"
        "   border: none;"
        "   border-radius: 4px;"
        "   padding: 8px 16px;"
        "   font-size: 14px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #F57C00;"
        "}"
    );
    connect(sendGroupImageBtn, &QPushButton::clicked, [this]() {
        QString filePath = QFileDialog::getOpenFileName(
            this,
            "选择图片",
            QDir::homePath(),
            "图片文件 (*.png *.jpg *.jpeg *.bmp *.gif)"
        );
        if (filePath.isEmpty()) return;

        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::warning(this, "错误", "无法打开图片文件！");
            return;
        }

        const qint64 MAX_IMAGE_SIZE = 5 * 1024 * 1024;
        if (file.size() > MAX_IMAGE_SIZE) {
            QMessageBox::warning(this, "警告",
                QString("图片过大（当前%1MB，最大支持5MB），请压缩后再发送！")
                .arg(QString::number(file.size() / 1024.0 / 1024.0, 'f', 2)));
            file.close();
            return;
        }

        QString fileName = QFileInfo(filePath).fileName();
        fileName = fileName.replace("|", "_").replace("\\", "_").replace("/", "_");

        QByteArray imageData = file.readAll();
        file.close();

        // ========== 核心修改：用“头部长度”替代 | 分隔符 ==========
        // 1. 拼接纯文本头部（GROUP|发送者|文件名|文件大小）
        QString headerText = QString("GROUP|%1|%2|%3")
            .arg(username)
            .arg(fileName)
            .arg(imageData.size());
        QByteArray headerData = headerText.toUtf8();

        // 2. 先写头部长度（4字节，固定长度，网络字节序）
        qint32 headerLen = headerData.size();
        QByteArray lenBytes;
        QDataStream lenStream(&lenBytes, QIODevice::WriteOnly);
        lenStream.setByteOrder(QDataStream::BigEndian); // 网络字节序
        lenStream << headerLen;

        // 3. 完整数据包：4字节头部长度 + 头部内容 + 图片二进制数据
        QByteArray packetData;
        packetData.append(lenBytes);       // 前4字节：头部长度
        packetData.append(headerData);     // 头部内容
        packetData.append(imageData);      // 图片二进制数据

        Packet packet;
        packet.type = IMAGE_TRANSFER;
        packet.data = packetData;
        sendPacket(packet);

        appendMessage(QString("[发送群聊图片：%1]").arg(fileName), true);
        });

    // 添加到按钮布局
    btnLayout->insertWidget(1, sendGroupImageBtn);


    // 右侧：在线用户列表
    QWidget* userListArea = new QWidget();
    QVBoxLayout* userListLayout = new QVBoxLayout(userListArea);
    userListLayout->setContentsMargins(10, 10, 10, 10);

    QLabel* userListLabel = new QLabel("在线用户");
    userListLabel->setStyleSheet("font-weight: bold; font-size: 16px;");
    userListLabel->setAlignment(Qt::AlignCenter);

    userListWidget = new QListWidget();
    userListWidget->setStyleSheet(R"(
        QListWidget {
            border: 1px solid #ddd;
            border-radius: 6px;
            padding: 5px;
            background: white;
        }
        QListWidget::item {
            padding: 5px;
            border-bottom: 1px solid #eee;
        }
        QListWidget::item:last {
            border-bottom: none;
        }
    )");

    userListWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(userListWidget, &QWidget::customContextMenuRequested, [this](const QPoint& pos) {
        QListWidgetItem* item = userListWidget->itemAt(pos);
        if (item) {
            QString targetUser = item->text();
            QMenu menu(this);

            QAction* privateChatAction = menu.addAction("💬 请求私聊");
            connect(privateChatAction, &QAction::triggered, [this, targetUser]() {
                // 发送私聊请求包，而非直接打开窗口
                Packet requestPacket;
                requestPacket.type = PRIVATE_CHAT_REQUEST;
                requestPacket.data = QString("%1|%2").arg(username).arg(targetUser).toUtf8();
                sendPacket(requestPacket);
                QMessageBox::information(this, "请求已发送", "私聊请求已发送，请等待对方回应～");
                });

            // 如果已有私聊窗口，添加关闭选项
            if (privateChatWindows.contains(targetUser)) {
                QAction* closePrivateAction = menu.addAction("❌ 关闭私聊窗口");
                connect(closePrivateAction, &QAction::triggered, [this, targetUser]() {
                    closePrivateChatWindow(targetUser);
                    });

                QAction* bringToFrontAction = menu.addAction("🔝 切换到私聊窗口");
                connect(bringToFrontAction, &QAction::triggered, [this, targetUser]() {
                    PrivateChatWindow* window = privateChatWindows.value(targetUser);
                    if (window) {
                        window->show();
                        window->activateWindow();
                        window->raise();
                    }
                    });
            }

            menu.exec(userListWidget->mapToGlobal(pos));
        }
        });

    userListLayout->addWidget(userListLabel);
    userListLayout->addWidget(userListWidget);
    userListArea->setMaximumWidth(150);

    // 添加到主布局
    chatMainLayout->addWidget(chatArea, 1);  // 聊天区域占大部分空间
    chatMainLayout->addWidget(userListArea); // 用户列表区域

    // 设置聊天区域最小宽度
    chatArea->setMinimumWidth(400);
}



void TCPClient::showPrivateChatWindowManager() {
    if (privateChatWindows.isEmpty()) {
        QMessageBox::information(this, "私聊窗口", "当前没有打开的私聊窗口");
        return;
    }

    QMenu menu(this);
    menu.setTitle("私聊窗口管理");

    foreach(const QString & targetUser, privateChatWindows.keys()) {
        PrivateChatWindow* window = privateChatWindows.value(targetUser);
        QAction* chatAction = menu.addAction(QString("💬 %1").arg(targetUser));
        connect(chatAction, &QAction::triggered, [window]() {
            if (window) {
                window->show();
                window->activateWindow();
                window->raise();
            }
            });
    }

    menu.addSeparator();
    QAction* closeAllAction = menu.addAction("❌ 关闭所有私聊窗口");
    connect(closeAllAction, &QAction::triggered, [this]() {
        QList<QString> keys = privateChatWindows.keys();
        foreach(const QString & key, keys) {
            closePrivateChatWindow(key);
        }
        });

    menu.exec(QCursor::pos());
}



// 切换到聊天界面
void TCPClient::switchToChatUI() {
    loginWidget->hide();
    chatWidget->show();
    appendMessage("登录成功，开始聊天吧～", false, true);
}




// 登录按钮点击事件
void TCPClient::onLoginClicked() {
    // 新增：如果Socket已连接/正在连接，先断开
    if (tcpSocket->state() != QTcpSocket::UnconnectedState) {
        tcpSocket->abort();
    }

    // 获取输入内容
    QString ip = ipEdit->text().trimmed();
    QString portStr = portEdit->text().trimmed();
    username = usernameEdit->text().trimmed();

    // 输入校验
    if (ip.isEmpty() || portStr.isEmpty() || username.isEmpty()) {
        QMessageBox::warning(this, "警告", "IP、端口、用户名不能为空！");
        return;
    }

    bool ok;
    quint16 port = portStr.toUShort(&ok);
    if (!ok) {
        QMessageBox::warning(this, "警告", "端口必须是数字！");
        return;
    }

    // 连接服务器
    tcpSocket->connectToHost(ip, port);
}




void TCPClient::appendMessage(const QString& message, bool isSelf, bool isSystem) {
    QTextCursor cursor = chatLogEdit->textCursor();
    cursor.movePosition(QTextCursor::End);

    if (isSystem) {
        // 系统消息：居中，蓝色，不加粗
        QTextBlockFormat blockFormat;
        QTextCharFormat charFormat;

        blockFormat.setAlignment(Qt::AlignCenter);
        charFormat.setForeground(QColor(0, 100, 200)); // 蓝色
        charFormat.setFontPointSize(10); // 稍小字号
        charFormat.setFontWeight(QFont::Normal); // 不加粗

        blockFormat.setTopMargin(12);
        blockFormat.setBottomMargin(12);

        cursor.insertBlock(blockFormat, charFormat);
        cursor.insertText(message);
    }
    else {
        // 聊天消息
        QTextBlockFormat blockFormat;
        QTextCharFormat timeFormat;
        QTextCharFormat contentFormat;

        // 设置对齐
        if (isSelf) {
            blockFormat.setAlignment(Qt::AlignRight);
            contentFormat.setBackground(QColor(220, 255, 220)); // 浅绿色
        }
        else {
            blockFormat.setAlignment(Qt::AlignLeft);
            contentFormat.setBackground(QColor(240, 240, 240)); // 浅灰色
        }

        blockFormat.setTopMargin(8);
        blockFormat.setBottomMargin(8);
        blockFormat.setRightMargin(10);
        blockFormat.setLeftMargin(10);

        // 插入时间
        timeFormat.setFontPointSize(10);
        timeFormat.setForeground(QColor(150, 150, 150)); // 灰色
        timeFormat.setFontWeight(QFont::Normal);

        QString timeStr = QDateTime::currentDateTime().toString("hh:mm:ss");

        cursor.insertBlock(blockFormat);
        cursor.setCharFormat(timeFormat);
        cursor.insertText(timeStr + "  ");

        // 插入消息内容
        contentFormat.setFontPointSize(13);
        contentFormat.setFontWeight(QFont::Normal); // 确保不加粗

        cursor.setCharFormat(contentFormat);
        if (isSelf) {
            cursor.insertText("[我] " + message);
        }
        else {
            // 对于他人消息，去掉加粗
            cursor.insertText(message);
        }
    }

    // 滚动到底部
    chatLogEdit->ensureCursorVisible();
}




// 发送消息按钮点击事件
void TCPClient::onSendMsgClicked() {
    QString msg = msgInputEdit->text().trimmed();
    if (msg.isEmpty()) {
        QMessageBox::warning(this, "警告", "消息不能为空！");
        return;
    }

    // 只处理群聊消息
    Packet packet;
    packet.type = CHAT_MESSAGE;
    packet.data = QString("%1|%2").arg(username).arg(msg).toUtf8();

    // 显示自己的消息
    appendMessage(msg, true, false);

    sendPacket(packet);
    msgInputEdit->clear();
}



// 更新用户列表
void TCPClient::updateUserList(const QStringList& users) {
    userListWidget->clear();
    foreach(const QString & user, users) {
        if (user != username) {  // 不显示自己
            userListWidget->addItem(user);
        }
    }
}




// 发送封装后的数据包
void TCPClient::sendPacket(const Packet& packet) {
    QByteArray sendData = packet.pack();
    tcpSocket->write(sendData);
}



// 连接服务器成功
void TCPClient::onConnected() {
    qDebug() << "已连接服务器，发送登录请求";

    Packet loginPacket;
    loginPacket.type = LOGIN_REQUEST;
    loginPacket.data = username.toUtf8();

    sendPacket(loginPacket);
}




// 断开连接
void TCPClient::onDisconnected() {
    chatLogEdit->append(QString("[%1] 与服务器断开连接！").arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
    // 补充：强制还原登录界面+清空变量
    chatWidget->hide();
    loginWidget->show();
    username.clear();
    usernameEdit->clear();
    // 重置Socket
    tcpSocket->reset();
}




// 接收服务器数据
void TCPClient::onReadyRead() {
    recvBuffer.append(tcpSocket->readAll());
    Packet packet;

    while (Packet::unpack(recvBuffer, packet)) {
        switch (packet.type) {
        case LOGIN_RESPONSE: {
            QString response = QString::fromUtf8(packet.data);
            if (response.startsWith("success")) {
                switchToChatUI();
            }
            else {
                QMessageBox::critical(this, "登录失败", response);
            }
            break;
        }
        case CHAT_MESSAGE: {
            QString data = QString::fromUtf8(packet.data);
            QStringList parts = data.split("|");  // 改为 | 分隔
            if (parts.size() >= 2) {
                QString sender = parts[0];
                QString message = parts.mid(1).join("|");  // 重新合并内容部分

                if (sender != username) {
                    appendMessage(QString("[%1] %2").arg(sender).arg(message), false, false);
                }
            }
            break;
        }

        case PRIVATE_CHAT_REQUEST: {
            QString data = QString::fromUtf8(packet.data);
            QStringList parts = data.split("|");
            if (parts.size() >= 2) {
                QString requester = parts[0]; // 发起者用户名
                QString receiver = parts[1]; // 接收者（自己）

                // 弹窗询问是否同意
                QMessageBox::StandardButton btn = QMessageBox::question(
                    this,
                    "私聊请求",
                    QString("%1 想要与你私聊，是否同意？").arg(requester),
                    QMessageBox::Yes | QMessageBox::No
                );

                // 发送响应包
                Packet responsePacket;
                responsePacket.type = PRIVATE_CHAT_RESPONSE;
                if (btn == QMessageBox::Yes) {
                    responsePacket.data = QString("%1|%2|agree").arg(receiver).arg(requester).toUtf8();
                    // 同意后直接打开私聊窗口
                    openPrivateChatWindow(requester);
                }
                else {
                    responsePacket.data = QString("%1|%2|refuse:暂时不便").arg(receiver).arg(requester).toUtf8();
                }
                sendPacket(responsePacket);
            }
            break;
        }

        case PRIVATE_CHAT_RESPONSE: {
            QString data = QString::fromUtf8(packet.data);
            QStringList parts = data.split("|");
            if (parts.size() >= 3) {
                QString originalReceiver = parts[0];  // 原接收者（22）
                QString originalRequester = parts[1]; // 原发起者（11）
                QString response = parts[2];          // agree/refuse

                // 关键修正：发起者（11）判断parts[1]是否等于自己，而非parts[0]
                if (originalRequester == username) { // 11的用户名 == parts[1]
                    if (response.startsWith("agree")) {
                        // 对方同意，打开私聊窗口（目标用户是originalReceiver，即22）
                        openPrivateChatWindow(originalReceiver);
                        QMessageBox::information(
                            this,
                            "私聊请求已同意",
                            QString("%1 已同意你的私聊请求，可以开始聊天了～").arg(originalReceiver)
                        );
                    }
                    else if (response.startsWith("refuse")) {
                        QString reason = response.mid(response.indexOf(":") + 1);
                        QMessageBox::warning(
                            this,
                            "私聊请求被拒绝",
                            QString("%1 拒绝了你的私聊请求。原因：%2").arg(originalReceiver).arg(reason)
                        );
                    }
                }
            }
            break;
        }


        case PRIVATE_CHAT_MESSAGE: {
            QString data = QString::fromUtf8(packet.data);
            QStringList parts = data.split("|");
            if (parts.size() >= 3) {
                QString sender = parts[0];
                QString receiver = parts[1];  // 接收者用户名
                QString content = parts.mid(2).join("|");

                // 确保消息是发给自己的
                if (receiver == username) {
                    // 检查是否有对应的私聊窗口
                    if (privateChatWindows.contains(sender)) {
                        PrivateChatWindow* window = privateChatWindows.value(sender);
                        // isSelf = false 表示对方消息，显示在左边
                        window->appendMessage(content, false);
                        window->show(); // 激活窗口
                        window->activateWindow();
                    }
                    else {
                        // 未同意私聊，收到消息提示用户
                        QMessageBox::information(
                            this,
                            "收到私聊消息",
                            QString("%1 给你发送了私聊消息，但你未同意私聊，请先发起/同意私聊请求").arg(sender)
                        );
                    }
                }
            }
            break;
        }

        case IMAGE_TRANSFER: {
            QByteArray allData = packet.data;
            // ========== 核心修改：先读4字节头部长度 ==========
            if (allData.size() < 4) { // 至少要有4字节长度
                QMessageBox::warning(this, "错误", "图片数据包过短");
                break;
            }

            // 1. 解析头部长度（4字节，网络字节序）
            QDataStream lenStream(allData.left(4));
            lenStream.setByteOrder(QDataStream::BigEndian);
            qint32 headerLen;
            lenStream >> headerLen;

            // 2. 校验头部长度是否合法
            if (headerLen <= 0 || headerLen >= allData.size() - 4) {
                QMessageBox::warning(this, "错误", "图片头部长度非法");
                break;
            }

            // 3. 拆分头部和图片数据
            QByteArray headerData = allData.mid(4, headerLen); // 4字节后取headerLen字节作为头部
            QByteArray imageData = allData.mid(4 + headerLen); // 剩余部分是图片数据
            QString headerStr = QString::fromUtf8(headerData);
            QStringList headerParts = headerStr.split("|");

            if (headerParts.isEmpty()) break;
            QString typeFlag = headerParts[0];

            // 群聊图片解析
            if (typeFlag == "GROUP") {
                if (headerParts.size() != 4) {
                    QMessageBox::warning(this, "错误",
                        QString("群聊图片头部格式错误（预期4段，实际%1段）").arg(headerParts.size()));
                    break;
                }
                QString sender = headerParts[1];
                QString fileName = headerParts[2];
                qint64 fileSize = headerParts[3].toLongLong();

                // 验证图片大小
                if (imageData.size() != fileSize) {
                    QMessageBox::warning(this, "警告",
                        QString("收到 %1 的群聊图片不完整（收到 %2 字节，预期 %3 字节）")
                        .arg(sender).arg(imageData.size()).arg(fileSize));
                    break;
                }

                // 显示消息+保存
                if (sender != username) {
                    appendMessage(QString("[%1 发送群聊图片：%2]").arg(sender).arg(fileName), false);
                    QMessageBox::StandardButton btn = QMessageBox::information(
                        this, "收到群聊图片",
                        QString("✅ %1 发送了群聊图片：%2\n是否立即保存？").arg(sender).arg(fileName),
                        QMessageBox::Yes | QMessageBox::No
                    );
                    if (btn == QMessageBox::Yes) {
                        QString savePath = QFileDialog::getSaveFileName(
                            this, "保存群聊图片", QDir::homePath() + "/" + fileName,
                            "图片文件 (*.png *.jpg *.jpeg *.bmp *.gif)"
                        );
                        if (!savePath.isEmpty()) {
                            QFile saveFile(savePath);
                            if (saveFile.open(QIODevice::WriteOnly)) {
                                saveFile.write(imageData);
                                saveFile.close();
                                QMessageBox::information(this, "成功", QString("图片已保存到：%1").arg(savePath));
                            }
                            else {
                                QMessageBox::warning(this, "错误", "无法保存图片文件！");
                            }
                        }
                    }
                }
            }
            // 私聊图片解析（同样替换成“长度+头部”的方式，逻辑和群聊一致）
            // 私聊图片解析
            else if (typeFlag == "PRIVATE") {
                if (headerParts.size() != 5) { // PRIVATE|发送者|接收者|文件名|文件大小
                    QMessageBox::warning(this, "错误", "私聊图片头部格式错误");
                    break;
                }
                QString sender = headerParts[1];
                QString receiver = headerParts[2];
                QString fileName = headerParts[3];
                qint64 fileSize = headerParts[4].toLongLong();

                if (receiver != username) break; // 只处理发给自己的

                // 验证文件大小
                if (imageData.size() != fileSize) {
                    QMessageBox::warning(this, "警告",
                        QString("收到 %1 的私聊图片不完整（收到 %2 字节，预期 %3 字节）")
                        .arg(sender).arg(imageData.size()).arg(fileSize));
                    break;
                }

                // 🔥 强制激活窗口+弹窗
                if (privateChatWindows.contains(sender)) {
                    PrivateChatWindow* window = privateChatWindows.value(sender);
                    window->displayImageThumbnail(imageData, fileName, false);
                    // 窗口置顶
                    window->show();
                    window->activateWindow();
                    window->raise();
                    // 强制提示保存
                    QMessageBox::information(this, "收到私聊图片",
                        QString("✅ %1 给你发送了私聊图片：%2\n点击【确定】保存").arg(sender).arg(fileName));
                    QString savePath = QFileDialog::getSaveFileName(
                        this, "保存私聊图片", QDir::homePath() + "/" + fileName,
                        "图片文件 (*.png *.jpg *.jpeg *.bmp *.gif)"
                    );
                    if (!savePath.isEmpty()) {
                        QFile saveFile(savePath);
                        if (saveFile.open(QIODevice::WriteOnly)) {
                            saveFile.write(imageData);
                            saveFile.close();
                            QMessageBox::information(this, "成功", QString("图片已保存到：%1").arg(savePath));
                        }
                        else {
                            QMessageBox::warning(this, "错误", "无法保存图片文件！");
                        }
                    }
                }
                else {
                    // 未打开私聊窗口时的处理
                    QMessageBox::StandardButton btn = QMessageBox::information(
                        this, "收到图片",
                        QString("%1 给你发送了私聊图片，但你未打开私聊窗口！\n是否打开窗口并保存？").arg(sender),
                        QMessageBox::Yes | QMessageBox::No
                    );
                    if (btn == QMessageBox::Yes) {
                        openPrivateChatWindow(sender);
                        QString savePath = QFileDialog::getSaveFileName(
                            this, "保存私聊图片", QDir::homePath() + "/" + fileName,
                            "图片文件 (*.png *.jpg *.jpeg *.bmp *.gif)"
                        );
                        if (!savePath.isEmpty()) {
                            QFile saveFile(savePath);
                            if (saveFile.open(QIODevice::WriteOnly)) {
                                saveFile.write(imageData);
                                saveFile.close();
                                QMessageBox::information(this, "成功", "图片保存成功！");
                            }
                        }
                    }
                }
            }
            else {
                QMessageBox::warning(this, "错误", "未知的图片传输类型");
            }
            break;
        }



        case USER_ONLINE: {
            QString newUser = QString::fromUtf8(packet.data);
            if (newUser != username) {  // 不显示自己的上线消息
                appendMessage(QString("🌟 用户 %1 上线").arg(newUser), false, true);

                // 添加到用户列表
                QList<QListWidgetItem*> items = userListWidget->findItems(newUser, Qt::MatchExactly);
                if (items.isEmpty()) {
                    userListWidget->addItem(newUser);
                }
            }
            break;
        }

        case USER_OFFLINE: {
            QString offlineUser = QString::fromUtf8(packet.data);
            if (offlineUser != username) {  // 不显示自己的下线消息
                appendMessage(QString("🔴 用户 %1 下线").arg(offlineUser), false, true);

                // 从用户列表中移除
                QList<QListWidgetItem*> items = userListWidget->findItems(offlineUser, Qt::MatchExactly);
                foreach(QListWidgetItem * item, items) {
                    delete userListWidget->takeItem(userListWidget->row(item));
                }
            }
            break;
        }

        case USER_LIST: {
            QString usersStr = QString::fromUtf8(packet.data);
            QStringList users = usersStr.split(",", Qt::SkipEmptyParts);
            updateUserList(users);
            break;
        }

        default:
            appendMessage("收到未知类型数据包", false);
            break;
        }
    }
}




// 图片保存
void TCPClient::saveImageToFile(const QByteArray& imageData, const QString& fileName) {
    QString savePath = QFileDialog::getSaveFileName(
        this, "保存图片", QDir::homePath() + "/" + fileName,
        "图片文件 (*.png *.jpg *.jpeg *.bmp *.gif)"
    );
    if (savePath.isEmpty()) return;

    QFile saveFile(savePath);
    if (saveFile.open(QIODevice::WriteOnly)) {
        saveFile.write(imageData);
        saveFile.close();
        QMessageBox::information(this, "成功", "图片保存成功！");
    }
    else {
        QMessageBox::warning(this, "错误", "无法保存图片文件！");
    }
}



//私聊窗口打开
void TCPClient::openPrivateChatWindow(const QString& targetUser) {
    // 检查窗口是否已存在
    if (privateChatWindows.contains(targetUser)) {
        PrivateChatWindow* window = privateChatWindows.value(targetUser);
        window->show();
        window->activateWindow();
        window->raise();  // 提到最前面
        return;
    }

    // 创建新窗口
    PrivateChatWindow* window = new PrivateChatWindow(username, targetUser, tcpSocket);
    window->setAttribute(Qt::WA_DeleteOnClose);

    // 设置窗口标志，确保不会遮挡主窗口
    window->setWindowFlags(window->windowFlags() | Qt::Window);

    connect(window, &PrivateChatWindow::windowClosed, this, &TCPClient::onPrivateWindowClosed);

    privateChatWindows.insert(targetUser, window);

    // 显示在主窗口旁边
    QPoint mainWindowPos = this->pos();
    int offsetX = privateChatWindows.size() * 30;  // 稍微偏移，避免完全重叠
    window->move(mainWindowPos.x() + this->width() + 10 + offsetX,
        mainWindowPos.y() + offsetX);
    window->show();
}


//私聊窗口关闭
void TCPClient::onPrivateWindowClosed(const QString& targetUsername) {
    PrivateChatWindow* window = privateChatWindows.take(targetUsername);
    if (window) {
        window->deleteLater();
    }
}



void TCPClient::closePrivateChatWindow(const QString& targetUser) {
    if (privateChatWindows.contains(targetUser)) {
        PrivateChatWindow* window = privateChatWindows.take(targetUser);
        window->close();
        window->deleteLater();
    }
}

// 网络错误处理
void TCPClient::onSocketError(QAbstractSocket::SocketError error) {
    Q_UNUSED(error);
    QMessageBox::critical(this, "网络错误", tcpSocket->errorString());
}