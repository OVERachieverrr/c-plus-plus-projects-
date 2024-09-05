#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <tchar.h>
#include <thread>
#include <vector>
#include <fstream>

using namespace std;
#pragma comment(lib, "ws2_32.lib")

class Server
{
public:
      Server(int pot) : port(pot) {}

    bool Initialize()
    {
        WSADATA wsaData;
        return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
    }

    bool Start()
    {
        if (!Initialize())
        {
            cout << "Failed setting up Windows sockets" << endl;
            return false;
        }

        Server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (Server_socket == INVALID_SOCKET)
        {
            cout << "Couldn't create socket" << endl;
            WSACleanup();
            return false;
        }

        sockaddr_in serveraddr;
        serveraddr.sin_family = AF_INET;
        serveraddr.sin_port = htons(port);

        if (InetPton(AF_INET, _T("0.0.0.0"), &serveraddr.sin_addr) != 1)
        {
            cout << "Setting address structure failed" << endl;
            Cleanup();
            return false;
        }

        if (bind(Server_socket, reinterpret_cast<sockaddr*>(&serveraddr), sizeof(serveraddr)) == SOCKET_ERROR)
        {
            cout << "Binding failed" << endl;
            Cleanup();
            return false;
        }

        if (listen(Server_socket, SOMAXCONN) == SOCKET_ERROR)
        {
            cout << "Listen failed" << endl;
            Cleanup();
            return false;
        }

        cout << "Server has started listening on port: " << port << endl;
        return true;
    }

    void Run()
    {
        while (true)
        {
            SOCKET clientSOCKET = accept(Server_socket, nullptr, nullptr);
            if (clientSOCKET == INVALID_SOCKET)
            {
                cout << "Invalid client socket" << endl;
                continue;
            }
            clients.push_back(clientSOCKET);
            thread t1(&Server::InteractWithClient, this, clientSOCKET);
            t1.detach();
        }
    }

    ~Server()
    {
        Cleanup();
    }

private:
    int port;
    SOCKET Server_socket;
    vector<SOCKET> clients;

    void Cleanup()
    {
        closesocket(Server_socket);
        WSACleanup();
    }

    void InteractWithClient(SOCKET clientSOCKET)
    {
        cout << "Client connected" << endl;
        char buffer[4096];

        while (true)
        {
            // Receive the header (1 byte) to determine the type of data
            char header;
            int bytesReceived = recv(clientSOCKET, &header, sizeof(header), 0);
            if (bytesReceived <= 0)
            {
                cout << "Client disconnected" << endl;
                break;
            }

            if (header == 1)  // Text message
            {
                bytesReceived = recv(clientSOCKET, buffer, sizeof(buffer), 0);
                if (bytesReceived <= 0)
                {
                    cout << "Client disconnected" << endl;
                    break;
                }
                string message(buffer, bytesReceived);

                // Relay the text message to all clients except the sender
                for (auto client : clients)
                {
                    if (client != clientSOCKET)
                    {
                        send(client, &header, sizeof(header), 0);  // Send header
                        send(client, message.c_str(), message.length(), 0);  // Send message
                    }
                }
            }
            else if (header == 2)  // Image data
            {
                // Receive the image size first (as a header)
                int imgSize;
                recv(clientSOCKET, reinterpret_cast<char*>(&imgSize), sizeof(imgSize), 0);

                // Now receive the image data in chunks
                vector<char> imgData(imgSize);
                int totalBytesReceived = 0;
                while (totalBytesReceived < imgSize)
                {
                    bytesReceived = recv(clientSOCKET, imgData.data() + totalBytesReceived, imgSize - totalBytesReceived, 0);
                    if (bytesReceived <= 0)
                    {
                        cout << "Client disconnected during image transfer" << endl;
                        break;
                    }
                    totalBytesReceived += bytesReceived;
                }

                // Relay the image data to all clients except the sender
                for (auto client : clients)
                {
                    if (client != clientSOCKET)
                    {
                        send(client, &header, sizeof(header), 0);  // Send header
                        send(client, reinterpret_cast<char*>(&imgSize), sizeof(imgSize), 0);  // Send image size
                        send(client, imgData.data(), imgSize, 0);  // Send image data
                    }
                }
            }
        }

        auto it = find(clients.begin(), clients.end(), clientSOCKET);
        if (it != clients.end())
        {
            clients.erase(it);
        }
        closesocket(clientSOCKET);
    }
};

int main()
{
    int port = 12345;
    Server server(port);

    if (!server.Start())
    {
        return 1;
    }

    server.Run();
    return 0;
}
