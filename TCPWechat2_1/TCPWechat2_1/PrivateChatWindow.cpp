#include "PrivateChatWindow.h"
#include <QDateTime>
#include <QMessageBox>
#include <QLabel>
#include <QFrame>
#include <QScrollBar>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QCloseEvent>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QImage>
#include <QTextImageFormat>


PrivateChatWindow::PrivateChatWindow(const QString& myUsername,
    const QString& targetUsername,
    QTcpSocket* socket,
    QWidget* parent)
    : QWidget(parent)
    , myUsername_(myUsername)
    , targetUsername_(targetUsername)
    , socket_(socket) {

    setWindowTitle(QString("私聊 - %1").arg(targetUsername));
    resize(600, 500);
    setAttribute(Qt::WA_DeleteOnClose);

    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // 左侧：聊天区域
    QWidget* chatArea = new QWidget();
    QVBoxLayout* chatLayout = new QVBoxLayout(chatArea);
    chatLayout->setContentsMargins(0, 0, 0, 0);

    chatHistory_ = new QTextEdit();
    chatHistory_->setReadOnly(true);
    chatHistory_->setStyleSheet(
        "QTextEdit {"
        "   background: #f9f9f9;"
        "   border: 1px solid #ddd;"
        "   border-radius: 5px;"
        "   padding: 10px;"
        "   font-size: 14px;"
        "}"
    );

    // 消息输入区
    QHBoxLayout* inputLayout = new QHBoxLayout();
    messageInput_ = new QLineEdit();
    messageInput_->setPlaceholderText("输入私聊消息...");

    sendBtn_ = new QPushButton("发送");
    sendBtn_->setStyleSheet(
        "QPushButton {"
        "   background-color: #2196F3;"
        "   color: white;"
        "   border: none;"
        "   border-radius: 4px;"
        "   padding: 8px 16px;"
        "   font-size: 14px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #0b7dda;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #0a6cbc;"
        "}"
    );

    QPushButton* closeBtn = new QPushButton("关闭");
    closeBtn->setStyleSheet(
        "QPushButton {"
        "   background-color: #f44336;"
        "   color: white;"
        "   border: none;"
        "   border-radius: 4px;"
        "   padding: 8px 16px;"
        "   font-size: 14px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #d32f2f;"
        "}"
    );

    QPushButton* sendImageBtn = new QPushButton("发送图片");
    sendImageBtn->setStyleSheet(
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

    // 修复：私聊发送图片点击事件（删除重复代码+修正变量）
    // 私聊发送图片点击事件
    connect(sendImageBtn, &QPushButton::clicked, [this]() {
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

        const qint64 MAX_IMAGE_SIZE = 5 * 1024 * 1024; // 5MB限制
        if (file.size() > MAX_IMAGE_SIZE) {
            QMessageBox::warning(this, "警告", "图片过大（最大支持5MB），请压缩后再发送！");
            file.close();
            return;
        }

        // 1. 获取并清理文件名
        QString fileName = QFileInfo(filePath).fileName();
        fileName = fileName.replace("|", "_").replace("\\", "_").replace("/", "_");

        // 2. 读取图片二进制数据
        QByteArray imageData = file.readAll();
        file.close();

        // 3. 拼接私聊头部文本（PRIVATE|发送者|接收者|文件名|文件大小）
        QString headerText = QString("PRIVATE|%1|%2|%3|%4")
            .arg(myUsername_)
            .arg(targetUsername_)
            .arg(fileName)
            .arg(imageData.size());
        QByteArray headerData = headerText.toUtf8();

        // 4. 打包4字节头部长度（网络字节序）
        qint32 headerLen = headerData.size();
        QByteArray lenBytes;
        QDataStream lenStream(&lenBytes, QIODevice::WriteOnly);
        lenStream.setByteOrder(QDataStream::BigEndian);
        lenStream << headerLen;

        // 5. 完整数据包：4字节长度 + 头部 + 二进制数据
        QByteArray packetData;
        packetData.append(lenBytes);       // 前4字节：头部长度
        packetData.append(headerData);     // 头部文本
        packetData.append(imageData);      // 图片二进制数据

        // 6. 发送数据包
        Packet packet;
        packet.type = IMAGE_TRANSFER;
        packet.data = packetData;

        if (socket_ && socket_->state() == QAbstractSocket::ConnectedState) {
            socket_->write(packet.pack());
            appendMessage(QString("[发送图片：%1]").arg(fileName), true);
        }
        else {
            QMessageBox::critical(this, "错误", "网络连接已断开！");
        }
    });

    connect(closeBtn, &QPushButton::clicked, this, &PrivateChatWindow::close);

    inputLayout->addWidget(messageInput_);
    inputLayout->addWidget(sendBtn_);
    inputLayout->addWidget(closeBtn);
    inputLayout->addWidget(sendImageBtn);

    chatLayout->addWidget(chatHistory_);
    chatLayout->addLayout(inputLayout);

    // 右侧：用户信息区域
    QWidget* userInfoArea = new QWidget();
    userInfoArea->setMaximumWidth(150);
    QVBoxLayout* userInfoLayout = new QVBoxLayout(userInfoArea);
    userInfoLayout->setContentsMargins(10, 10, 10, 10);

    QLabel* userInfoLabel = new QLabel("用户信息");
    userInfoLabel->setStyleSheet("font-weight: bold; font-size: 16px; color: #2196F3;");
    userInfoLabel->setAlignment(Qt::AlignCenter);

    QLabel* targetUserLabel = new QLabel(targetUsername);
    targetUserLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #4CAF50; padding: 10px;");
    targetUserLabel->setAlignment(Qt::AlignCenter);

    QLabel* statusLabel = new QLabel("🟢 在线");
    statusLabel->setStyleSheet("font-size: 12px; color: #666; padding: 5px;");
    statusLabel->setAlignment(Qt::AlignCenter);

    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setStyleSheet("color: #ddd;");

    QLabel* tipLabel = new QLabel("私聊消息仅您和 " + targetUsername + " 可见");
    tipLabel->setWordWrap(true);
    tipLabel->setStyleSheet("font-size: 11px; color: #888; padding: 10px; background: #f0f0f0; border-radius: 5px;");

    userInfoLayout->addWidget(userInfoLabel);
    userInfoLayout->addWidget(targetUserLabel);
    userInfoLayout->addWidget(statusLabel);
    userInfoLayout->addWidget(line);
    userInfoLayout->addWidget(tipLabel);
    userInfoLayout->addStretch();

    mainLayout->addWidget(chatArea, 1);
    mainLayout->addWidget(userInfoArea);

    connect(sendBtn_, &QPushButton::clicked, this, &PrivateChatWindow::onSendClicked);
    connect(messageInput_, &QLineEdit::returnPressed, this, &PrivateChatWindow::onSendClicked);
}






PrivateChatWindow::~PrivateChatWindow() {
    emit windowClosed(targetUsername_);
}



void PrivateChatWindow::appendMessage(const QString& message, bool isSelf) {
    QTextCursor cursor = chatHistory_->textCursor();
    cursor.movePosition(QTextCursor::End);

    // 获取当前时间
    QString timeStr = QDateTime::currentDateTime().toString("HH:mm");

    // 创建块格式
    QTextBlockFormat blockFormat;

    // 设置对齐方式
    if (isSelf) {
        blockFormat.setAlignment(Qt::AlignRight);
    }
    else {
        blockFormat.setAlignment(Qt::AlignLeft);
    }

    // 设置边距
    blockFormat.setTopMargin(8);
    blockFormat.setBottomMargin(8);
    blockFormat.setRightMargin(10);
    blockFormat.setLeftMargin(10);

    // 插入新块
    cursor.insertBlock(blockFormat);

    // 插入时间戳和消息内容（使用简单的格式）
    QTextCharFormat charFormat;
    charFormat.setFontPointSize(13);

    // 时间戳格式
    QTextCharFormat timeFormat;
    timeFormat.setFontPointSize(10);
    timeFormat.setForeground(QColor(150, 150, 150)); // 灰色

    // 先插入时间戳
    cursor.setCharFormat(timeFormat);
    if (isSelf) {
        cursor.insertText("我 " + timeStr + " ");
    }
    else {
        cursor.insertText(targetUsername_ + " " + timeStr + " ");
    }

    // 再插入消息内容（带背景色）
    if (isSelf) {
        charFormat.setBackground(QColor(220, 255, 220)); // 使用群聊的浅绿色
        charFormat.setForeground(Qt::black);
    }
    else {
        charFormat.setBackground(QColor(240, 240, 240)); // 浅灰色
        charFormat.setForeground(Qt::black);
    }

    cursor.setCharFormat(charFormat);
    cursor.insertText(message);

    // 滚动到底部
    chatHistory_->ensureCursorVisible();
}


// 实现图片预览功能（添加在 appendMessage 函数之后）
void PrivateChatWindow::displayImageThumbnail(const QByteArray& imageData, const QString& fileName, bool isSelf) {
    QTextCursor cursor = chatHistory_->textCursor();
    cursor.movePosition(QTextCursor::End);

    appendMessage(QString("[%1图片：%2]").arg(isSelf ? "发送" : "收到").arg(fileName), isSelf);

    QImage image;
    if (image.loadFromData(imageData)) {
        QImage scaledImage = image.scaled(200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QTextImageFormat imageFormat;

        imageFormat.setName(QString("data:image/%1;base64,%2")
            .arg(fileName.split(".").last().toLower())
            .arg(QString(imageData.toBase64())));
        imageFormat.setWidth(scaledImage.width());
        imageFormat.setHeight(scaledImage.height());

        QTextBlockFormat blockFormat;
        blockFormat.setAlignment(isSelf ? Qt::AlignRight : Qt::AlignLeft);
        blockFormat.setTopMargin(5);
        blockFormat.setBottomMargin(15);
        cursor.insertBlock(blockFormat);
        cursor.insertImage(imageFormat);

        chatHistory_->ensureCursorVisible();
    }
    else {
        appendMessage(QString("[%1图片加载失败：%2]").arg(isSelf ? "发送" : "收到").arg(fileName), isSelf);
    }
}




void PrivateChatWindow::onSendClicked() {
    QString msg = messageInput_->text().trimmed();
    if (msg.isEmpty()) {
        QMessageBox::warning(this, "警告", "消息不能为空！");
        return;
    }

    // 发送私聊消息包
    Packet packet;
    packet.type = PRIVATE_CHAT_MESSAGE;
    // 格式：发送者|接收者|消息内容
    packet.data = QString("%1|%2|%3").arg(myUsername_).arg(targetUsername_).arg(msg).toUtf8();

    QByteArray sendData = packet.pack();
    if (socket_ && socket_->state() == QAbstractSocket::ConnectedState) {
        socket_->write(sendData);
    }
    else {
        QMessageBox::critical(this, "错误", "网络连接已断开！");
        return;
    }

    // 显示自己的消息（isSelf = true，显示在右边）
    appendMessage(msg, true);
    messageInput_->clear();
}




void PrivateChatWindow::closeEvent(QCloseEvent* event) {
    emit windowClosed(targetUsername_);
    event->accept();
}

