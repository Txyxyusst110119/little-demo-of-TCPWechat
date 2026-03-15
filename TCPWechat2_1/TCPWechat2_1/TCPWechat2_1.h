#pragma once

#include <QtWidgets/QWidget>
#include "ui_TCPWechat2_1.h"

class TCPWechat2_1 : public QWidget
{
    Q_OBJECT

public:
    TCPWechat2_1(QWidget *parent = nullptr);
    ~TCPWechat2_1();

private:
    Ui::TCPWechat2_1Class ui;
};

