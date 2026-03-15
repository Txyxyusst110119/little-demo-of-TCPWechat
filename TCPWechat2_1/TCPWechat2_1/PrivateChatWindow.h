#ifndef PRIVATECHATWINDOW_H
#define PRIVATECHATWINDOW_H

#include <QWidget>
#include <QTcpSocket>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCloseEvent>
#include <QLabel>        // 新增：用于 QLabel
#include <QFrame>        // 新增：用于 QFrame（装饰线）
#include <QScrollBar>    // 新增：用于滚动条
#include "Packet.h"

class PrivateChatWindow : public QWidget {
    Q_OBJECT
public:
    PrivateChatWindow(const QString& myUsername,
        const QString& targetUsername,
        QTcpSocket* socket,
        QWidget* parent = nullptr);
    ~PrivateChatWindow();

    QString getTargetUsername() const { return targetUsername_; }

    void appendMessage(const QString& message, bool isSelf);

    void displayImageThumbnail(const QByteArray& imageData, const QString& fileName, bool isSelf);

signals:
    void windowClosed(const QString& targetUsername);

private slots:
    void onSendClicked();


protected:
    void closeEvent(QCloseEvent* event) override;


private:


    QString myUsername_;
    QString targetUsername_;
    QTcpSocket* socket_;

    QTextEdit* chatHistory_;
    QLineEdit* messageInput_;
    QPushButton* sendBtn_;
};

#endif // PRIVATECHATWINDOW_H
