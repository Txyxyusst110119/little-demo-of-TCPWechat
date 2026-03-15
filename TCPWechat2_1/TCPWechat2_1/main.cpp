#include <QtWidgets/QApplication>
#include <QMessageBox>
#include <QPushButton>

#include "TCPServer.h"
#include "TCPClient.h"

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);

    QMessageBox box;
    box.setWindowTitle("模式选择");
    box.setText("请选择运行模式：");

    QPushButton* serverBtn = box.addButton("服务器模式", QMessageBox::AcceptRole);
    QPushButton* clientBtn = box.addButton("客户端模式", QMessageBox::RejectRole);

    box.exec();

    if (box.clickedButton() == serverBtn) {
        // ⭐ 使用 new，避免局部变量提前构造 socket，避免重复 socket notifier
        TCPServer* server = new TCPServer();
        server->show();
    }
    else {
        TCPClient* client = new TCPClient();
        client->show();
    }

    return a.exec();
}
