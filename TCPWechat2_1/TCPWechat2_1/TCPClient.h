#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QWidget>
#include <QTcpSocket>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QDateTime>
#include <QListWidget>
#include <QScrollBar>
#include <QMenu>        
#include <QAction>
#include <QMap>          // 新增：管理私聊窗口
#include "Packet.h"
#include "CRC32.h"
#include "PrivateChatWindow.h"  // 新增

class TCPClient : public QWidget {
    Q_OBJECT
public:
    TCPClient(QWidget* parent = nullptr);
    ~TCPClient()= default;

    void handlePrivateMessage(const QString& sender, const QString& receiver, const QString& content);

private slots:
    void onLoginClicked();
    void onSendMsgClicked();
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onSocketError(QAbstractSocket::SocketError error);

    void onPrivateWindowClosed(const QString& targetUsername);

private:
    void sendPacket(const Packet& packet);
    void initLoginUI();
    void initChatUI();
    void switchToChatUI();
    void appendMessage(const QString& message, bool isSelf = false, bool isSystem = false);
    void updateUserList(const QStringList& users);
   
    // 私聊窗口管理函数声明
    void openPrivateChatWindow(const QString& targetUser);
    void closePrivateChatWindow(const QString& targetUser);
    void cleanupOnLogout();  // 可选的清理函数
    void saveImageToFile(const QByteArray& imageData, const QString& fileName);
    void showPrivateChatWindowManager();
    // 成员变量
    QTcpSocket* tcpSocket;
    QString username;

    // 登录界面控件
    QLineEdit* ipEdit;
    QLineEdit* portEdit;
    QLineEdit* usernameEdit;
    QPushButton* loginBtn;

    // 聊天界面控件
    QTextEdit* chatLogEdit;
    QLineEdit* msgInputEdit;
    QPushButton* sendMsgBtn;
    QPushButton* logoutBtn;
    QListWidget* userListWidget;

    // 界面布局
    QWidget* loginWidget;
    QWidget* chatWidget;
    QByteArray recvBuffer;

    // 新增：私聊窗口管理
    QMap<QString, PrivateChatWindow*> privateChatWindows;  // key: 对方用户名, value: 窗口指针

    QString currentPrivateTarget;
};

#endif // TCPCLIENT_H