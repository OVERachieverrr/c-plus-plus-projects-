#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <vector>
#include <algorithm>
#include <cstring>

using namespace std;

class Server
{
public:
    Server(int pot) : port(pot) {}

    bool Initialize()
    {
        // No need for WSAStartup on Linux
        return true;
    }

    bool Start()
    {
        if (!Initialize())
        {
            cout << "Failed to initialize server" << endl;
            return false;
        }

        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket < 0)
        {
            cout << "Couldn't create socket" << endl;
            return false;
        }

        sockaddr_in serveraddr;
        serveraddr.sin_family = AF_INET;
        serveraddr.sin_port = htons(port);
        serveraddr.sin_addr.s_addr = INADDR_ANY;

        if (bind(server_socket, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0)
        {
            cout << "Binding failed" << endl;
            Cleanup();
            return false;
        }

        if (listen(server_socket, SOMAXCONN) < 0)
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
            int client_socket = accept(server_socket, nullptr, nullptr);
            if (client_socket < 0)
            {
                cout << "Invalid client socket" << endl;
                continue;
            }
            clients.push_back(client_socket);
            thread t1(&Server::InteractWithClient, this, client_socket);
            t1.detach();
        }
    }

    ~Server()
    {
        Cleanup();
    }

private:
    int port;
    int server_socket;
    vector<int> clients;

    void Cleanup()
    {
        close(server_socket);
    }

    void InteractWithClient(int client_socket)
    {
        cout << "Client connected" << endl;
        char buffer[4096];

        while (true)
        {
            char header;
            int bytesReceived = recv(client_socket, &header, sizeof(header), 0);
            if (bytesReceived <= 0)
            {
                cout << "Client disconnected" << endl;
                break;
            }

            if (header == 1)  // Text message
            {
                bytesReceived = recv(client_socket, buffer, sizeof(buffer), 0);
                if (bytesReceived <= 0)
                {
                    cout << "Client disconnected" << endl;
                    break;
                }
                string message(buffer, bytesReceived);

                for (auto client : clients)
                {
                    send(client, &header, sizeof(header), 0);  // Send header
                    send(client, message.c_str(), message.length(), 0);  // Send message
                }
            }
            else if (header == 2)  // Image data
            {
                int imgSize;
                recv(client_socket, reinterpret_cast<char*>(&imgSize), sizeof(imgSize), 0);

                vector<char> imgData(imgSize);
                int totalBytesReceived = 0;
                while (totalBytesReceived < imgSize)
                {
                    bytesReceived = recv(client_socket, imgData.data() + totalBytesReceived, imgSize - totalBytesReceived, 0);
                    if (bytesReceived <= 0)
                    {
                        cout << "Client disconnected during image transfer" << endl;
                        break;
                    }
                    totalBytesReceived += bytesReceived;
                }

                for (auto client : clients)
                {
                    send(client, &header, sizeof(header), 0);  // Send header
                    send(client, reinterpret_cast<char*>(&imgSize), sizeof(imgSize), 0);  // Send image size
                    send(client, imgData.data(), imgSize, 0);  // Send image data
                }
            }
        }

        auto it = find(clients.begin(), clients.end(), client_socket);
        if (it != clients.end())
        {
            clients.erase(it);
        }
        close(client_socket);
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
