# TCP Chatroom

This project implements a full-featured TCP-based chat system with a clean, intuitive UI and stable network communication. It follows object-oriented design principles and modular architecture, making the code maintainable and extensible. Ideal for demonstrating proficiency in Qt development, network programming, and multi-threaded application design.

## Features

- Multi-client chat support
- One-on-one conversations via right-click menu (independent windows)
- Group Chat support
- Server-client architecture
- Message broadcasting
- Basic command handling
- Message Styling: Self-messages aligned right (light green background), others' messages aligned left (light gray background)
- Image Handling: Preview thumbnails + save to local storage functionality

## Technologies

- C++
- TCP Socket
- Windows

## How to Run

First,you should have Qt 5.15 or later (compatible with Qt 6.x)
- 1.Clone the repository
- 2.Open the project in Qt Creator
- 3.Select src/TCP-ChatRoom.pro (create this .pro file if missing—see template below)
-- Configure build settings:
-- Select your Qt version and compiler
-- Set build directory (e.g., build/)
- 4.Click "Build" to compile the project

### Compile

g++ server.cpp -o server
g++ client.cpp -o client

### Run Server

./server

### Run Client

./client
