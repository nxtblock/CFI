#include <winsock2.h>
#include <iostream>
#include <thread>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")

using namespace std;

#define BUF_SIZE 1024

// 设置控制台颜色
void setColor(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

// 清屏
void clearScreen() {
    system("cls");
}

// 打印欢迎界面
void printWelcome() {
    setColor(11); // 浅青色
    cout << "====================================" << endl;
    cout << "      Chat_For_ICPC" << endl;
    cout << "====================================" << endl;
    setColor(7); // 恢复默认
}

SOCKET clientSocket;

// 接收消息线程
void receiveMessages() {
    char buffer[BUF_SIZE];
    while (true) {
        memset(buffer, 0, BUF_SIZE);
        int bytesReceived = recv(clientSocket, buffer, BUF_SIZE, 0);
        
        if (bytesReceived <= 0) {
            cout << "与服务器的连接已断开" << endl;
            closesocket(clientSocket);
            WSACleanup();
            exit(1);
        }
        
        string message(buffer, bytesReceived);
        
        // 系统消息用不同颜色显示
        if (message.find("[系统通知]") != string::npos) {
            setColor(12); // 红色
        } else if (message.find("加入了聊天室") != string::npos ||
                   message.find("离开了聊天室") != string::npos) {
            setColor(10); // 绿色
        } else {
            setColor(7); // 默认
        }
        
        cout << message << endl;
        setColor(7); // 恢复默认
    }
}

int main() {
    // 设置控制台标题
    SetConsoleTitle("Chat_For_ICPC");
    
    clearScreen();
    printWelcome();
    
    // 初始化Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        setColor(12);
        cerr << "WSAStartup失败" << endl;
        setColor(7);
        return 1;
    }
    
    // 创建socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        setColor(12);
        cerr << "创建socket失败" << endl;
        setColor(7);
        WSACleanup();
        return 1;
    }
    
    // 输入服务器IP
    string serverIP;
    setColor(14); // 黄色
    cout << "请输入服务器IP (默认: 127.0.0.1): ";
    setColor(7); // 默认
    getline(cin, serverIP);
    if (serverIP.empty()) {
        serverIP = "127.0.0.1";
    }
    
    // 连接服务器
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(serverIP.c_str());
    serverAddr.sin_port = htons(12345);
    
    setColor(11);
    cout << "正在连接服务器..." << endl;
    setColor(7);
    
    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        setColor(12);
        cerr << "连接服务器失败" << endl;
        setColor(7);
        closesocket(clientSocket);
        WSACleanup();
        system("pause");
        return 1;
    }
    
    setColor(10);
    cout << "已成功连接服务器" << endl;
    setColor(7);
    
    // 输入用户名
    string username;
    while (true) {
        setColor(14);
        cout << "请输入您的用户名: ";
        setColor(7);
        getline(cin, username);
        
        if (username.empty()) {
            setColor(12);
            cout << "用户名不能为空" << endl;
            setColor(7);
            continue;
        }
        
        if (username.find(' ') != string::npos) {
            setColor(12);
            cout << "用户名不能包含空格" << endl;
            setColor(7);
            continue;
        }
        
        break;
    }
    
    // 发送用户名到服务器
    send(clientSocket, username.c_str(), username.length(), 0);
    
    setColor(10);
    cout << "欢迎, " << username << "! 输入消息开始聊天" << endl;
    cout << "输入 /exit 退出, /list 查看在线用户" << endl;
    setColor(7);
    
    // 启动接收消息线程
    thread(receiveMessages).detach();
    
	while (true) {//发送消息
		string message;
		getline(cin, message);
		
		if (message == "/exit") {
			send(clientSocket, message.c_str(), message.length(), 0);
			break;
		} else if (!message.empty()) {
			static DWORD lastSendTime = 0;  // 记录上次发送时间
			DWORD currentTime = GetTickCount();
			
			// 计算距离上次发送的时间间隔
			DWORD timeSinceLastSend = currentTime - lastSendTime;
			
			if (timeSinceLastSend < 1000) {  //验证，防止刷屏
				int remainingTime = 500 - timeSinceLastSend;
				setColor(12); // 红色
				
				MessageBox(0,"Too fast,try again","Wait",MB_OK);
				
				setColor(7); // 恢复默认
				continue;
			}
			
			// 发送消息并更新最后发送时间
			send(clientSocket, message.c_str(), message.length(), 0);
			lastSendTime = currentTime;
		}
	}
    
    // 清理
    closesocket(clientSocket);
    WSACleanup();
    return 0;
}
