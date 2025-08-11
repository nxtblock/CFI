#include <winsock2.h>
#include <iostream>
#include <thread>
#include <unordered_map>
#include <mutex>
#include <vector>
#include <algorithm>
#pragma comment(lib, "ws2_32.lib")

#define PORT 12345
#define BUF_SIZE 1024

using namespace std;

// 全局变量
unordered_map<string, SOCKET> clients;  // 存储用户名和对应的socket
mutex clientsMutex;                     // 保护clients的互斥锁
vector<string> chatHistory;             // 聊天历史记录

// 广播消息给所有客户端
void broadcast(const string& message, const string& sender = "") {
    lock_guard<mutex> lock(clientsMutex);
    
    string formattedMsg;
    if (!sender.empty()) {
        formattedMsg = sender + ": " + message;
        // 保存到聊天历史
        chatHistory.push_back(formattedMsg);
    } else {
        formattedMsg = "[系统通知] " + message;
    }
    
    for (const auto& client : clients) {
        if (client.first != sender) {  // 不发送给发送者自己
            send(client.second, formattedMsg.c_str(), formattedMsg.length(), 0);
        }
    }
    
    // 在服务器端也显示消息
    cout << formattedMsg << endl;
}

// 发送聊天历史给新连接的客户端
void sendHistory(SOCKET clientSocket) {
    if (chatHistory.empty()) return;
    
    string historyMsg = "\n=== 聊天历史 ===\n";
    for (const auto& msg : chatHistory) {
        historyMsg += msg + "\n";
    }
    historyMsg += "===============\n";
    
    send(clientSocket, historyMsg.c_str(), historyMsg.length(), 0);
}

// 处理客户端连接
void handleClient(SOCKET clientSocket) {
    char buffer[BUF_SIZE] = {0};
    int bytesReceived;
    string username;

    // 获取用户名
    bytesReceived = recv(clientSocket, buffer, BUF_SIZE, 0);
    if (bytesReceived <= 0) {
        closesocket(clientSocket);
        return;
    }
    
    username = buffer;
    
    // 检查用户名是否已存在
    {
        lock_guard<mutex> lock(clientsMutex);
        if (clients.find(username) != clients.end()) {
            string msg = "[错误] 该用户名已存在，请重新连接并使用其他名字";
            send(clientSocket, msg.c_str(), msg.length(), 0);
            closesocket(clientSocket);
            return;
        }
        clients[username] = clientSocket;
    }
    
    // 欢迎消息
    string welcomeMsg = username + " 加入了聊天室!";
    broadcast(welcomeMsg);
    
    // 发送聊天历史给新用户
    sendHistory(clientSocket);
    
    cout << "[系统] " << username << " 加入了聊天室" << endl;
    
    // 接收消息循环
    while (true) {
        memset(buffer, 0, BUF_SIZE);
        bytesReceived = recv(clientSocket, buffer, BUF_SIZE, 0);
        
        if (bytesReceived <= 0) {
            break;
        }
        
        string message(buffer);
        
        // 处理特殊命令
        if (message == "/exit") {
            break;
        } else if (message == "/list") {
            string userList = "当前在线用户: ";
            lock_guard<mutex> lock(clientsMutex);
            for (const auto& client : clients) {
                userList += client.first + ", ";
            }
            userList = userList.substr(0, userList.length() - 2); // 移除最后的逗号和空格
            send(clientSocket, userList.c_str(), userList.length(), 0);
        } else {
            // 广播普通消息
            broadcast(message, username);
        }
    }
    
    // 客户端断开连接
    {
        lock_guard<mutex> lock(clientsMutex);
        clients.erase(username);
    }
    
    string leaveMsg = username + " 离开了聊天室";
    broadcast(leaveMsg);
    cout << "[系统] " << username << " 离开了聊天室" << endl;
    
    closesocket(clientSocket);
}

// 服务器命令处理
void serverCommandLoop() {
    string command;
    while (true) {
        getline(cin, command);
        
        if (command == "/exit") {
            broadcast("服务器即将关闭，所有连接将被终止");
            exit(0);
        } else if (command == "/list") {
            lock_guard<mutex> lock(clientsMutex);
            cout << "当前在线用户 (" << clients.size() << "):" << endl;
            for (const auto& client : clients) {
                cout << "- " << client.first << endl;
            }
        } else if (!command.empty()) {
            broadcast(command); // 作为系统消息广播
        }
    }
}

int main() {
	
	SetConsoleTitle("Chat_for_ICPC(Server)");
	
    // 初始化Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup失败" << endl;
        return 1;
    }
    
    // 创建socket
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        cerr << "创建socket失败" << endl;
        WSACleanup();
        return 1;
    }
    
    // 绑定socket
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);
    
    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "绑定失败" << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    
    // 监听
    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "监听失败" << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    
    cout << "聊天服务器已启动，监听端口 " << PORT << endl;
    cout << "可用命令:\n/list - 列出在线用户\n/exit - 关闭服务器\n直接输入消息 - 广播系统消息\n";
    
    // 启动服务器命令处理线程
    thread(serverCommandLoop).detach();
    
    // 接受客户端连接
    while (true) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            cerr << "接受连接失败" << endl;
            continue;
        }
        
        thread(handleClient, clientSocket).detach();
    }
    
    // 清理 (通常不会执行到这里)
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
