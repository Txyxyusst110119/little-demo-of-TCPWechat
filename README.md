# TCP Chatroom

该项目实现了一套功能完备的基于 TCP 的聊天系统，具备简洁直观的用户界面和稳定的网络通信能力。它遵循面向对象设计原则和模块化架构，使得代码具有可维护性和可扩展性。非常适合用于展示在 Qt 开发、网络编程和多线程应用设计方面的熟练程度。

This project implements a full-featured TCP-based chat system with a clean, intuitive UI and stable network communication. It follows object-oriented design principles and modular architecture, making the code maintainable and extensible. Ideal for demonstrating proficiency in Qt development, network programming, and multi-threaded application design.

## Features

- 多客户端聊天支持 Multi-client chat support
- 私聊模式 One-on-one conversations via right-click menu (independent windows)
- 群聊天 Group Chat support
- 服务器-客户端架构 Server-client architecture
- 消息广播 Message broadcasting
- 基本命令处理功能 Basic command handling
- 聊天框风格 Message Styling: Self-messages aligned right (light green background), others' messages aligned left (light gray background)
- 照片传输 Image Handling: Preview thumbnails + save to local storage functionality
  

## Technologies

博主主要是在win11系统上用vs2022来开发的，语言是中文版的。
- C++
- qt
- Windows

## How to Run

首先你得确保你的电脑上有qt，最好还有vs2022。
克隆完这个文件后，点击TCPWechat2_1.exe就行。
先开启一个服务器，然后再开启客户端。
First,you should have Qt 5.15 or later (compatible with Qt 6.x)
- 1.Clone the repository
- 2.click TCPWechat2_1.exe
- 3.you should first click Server，then click Client
