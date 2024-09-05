#include <wx/wx.h>
#include <wx/sizer.h>
#include <wx/textdlg.h> 
#include <wx/filedlg.h> 
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <thread>
#include <string>
#include <fstream>
#include <vector>
#include <iostream>

using namespace std;

#pragma comment(lib, "ws2_32.lib")

class ChatClient {
public:
    ChatClient(const string& serverAddress, int port)
        : serverAddress(serverAddress), port(port) {}

    bool Initialize() {
        WSADATA wsaData;
        return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
    }

    bool Connect() {
        s = socket(AF_INET, SOCK_STREAM, 0);
        if (s == INVALID_SOCKET) {
            return false;
        }

        sockaddr_in serveraddr;
        serveraddr.sin_family = AF_INET;
        serveraddr.sin_port = htons(port);
        inet_pton(AF_INET, serverAddress.c_str(), &(serveraddr.sin_addr));

        if (connect(s, reinterpret_cast<sockaddr*>(&serveraddr), sizeof(serveraddr)) == SOCKET_ERROR) {
            closesocket(s);
            WSACleanup();
            return false;
        }

        return true;
    }

    void Start(const string& name, wxTextCtrl* output) {
        this->name = name;
        this->output = output;

        thread ReceiverThread(&ChatClient::ReceiveMessage, this);
        ReceiverThread.detach();
    }

    void SendMessage(const string& message) {
        string msg = name + ": " + message;

        char header = 1;
        send(s, &header, sizeof(header), 0);
        send(s, msg.c_str(), msg.length(), 0);
    }

    void SendPhoto(const string& imagePath) {
        ifstream imageFile(imagePath, ios::binary);
        if (!imageFile.is_open()) {
            output->AppendText("Failed to open the image file!\n");
            return;
        }

        vector<char> imageData((istreambuf_iterator<char>(imageFile)), istreambuf_iterator<char>());
        imageFile.close();

        char header = 2;
        send(s, &header, sizeof(header), 0);

        int imgSize = imageData.size();
        send(s, reinterpret_cast<char*>(&imgSize), sizeof(imgSize), 0);

        send(s, imageData.data(), imgSize, 0);

        output->AppendText("Image sent successfully!\n");
    }

private:
    SOCKET s;
    string serverAddress;
    int port;
    string name;
    wxTextCtrl* output;

    void ReceiveMessage() {
        char buffer[4096];
        while (true) {
            char header;
            int bytesReceived = recv(s, &header, sizeof(header), 0);
            if (bytesReceived <= 0) {
                output->AppendText("Disconnected\n");
                break;
            }

            if (header == 1) {
                int recvlenght = recv(s, buffer, sizeof(buffer), 0);
                if (recvlenght > 0) {
                    string msg(buffer, recvlenght);
                    wxString wxMsg = wxString::FromUTF8(msg.c_str());
                    output->AppendText(wxMsg + "\n");
                }
            }
            else if (header == 2) {
                int imgSize;
                recv(s, reinterpret_cast<char*>(&imgSize), sizeof(imgSize), 0);

                vector<char> imgData(imgSize);
                int totalBytesReceived = 0;
                while (totalBytesReceived < imgSize) {
                    bytesReceived = recv(s, imgData.data() + totalBytesReceived, imgSize - totalBytesReceived, 0);
                    if (bytesReceived <= 0) {
                        output->AppendText("Disconnected during image transfer\n");
                        break;
                    }
                    totalBytesReceived += bytesReceived;
                }

                ofstream outputImage("received_image.jpg", std::ios::binary);
                outputImage.write(imgData.data(), imgSize);
                outputImage.close();

                output->AppendText("Image received and saved as 'received_image.jpg'\n");
            }
        }

        closesocket(s);
        WSACleanup();
    }
};

class ChatFrame : public wxFrame {
public:
    ChatFrame(const wxString& title, const wxColour& bgColor, const wxString& userName)
        : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(400, 600)),
        chatDisplay(nullptr), messageInput(nullptr), userName(userName) {

        SetBackgroundColour(bgColor);

        wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);


        wxStaticText* header = new wxStaticText(this, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
        header->SetFont(wxFont(30, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
        header->SetForegroundColour(*wxWHITE);

        wxStaticText* connectionStatus = new wxStaticText(this, wxID_ANY, "Connected to localhost", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
        connectionStatus->SetForegroundColour(*wxWHITE);

        mainSizer->Add(header, 0, wxEXPAND | wxALL, 10);
        mainSizer->Add(connectionStatus, 0, wxALIGN_CENTER | wxALL, 5);

        chatDisplay = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
        chatDisplay->SetBackgroundColour(wxColour(255, 255, 255));
        chatDisplay->SetForegroundColour(wxColour(0, 0, 0));

        mainSizer->Add(chatDisplay, 1, wxEXPAND | wxALL, 10);


        wxBoxSizer* inputSizer = new wxBoxSizer(wxHORIZONTAL);
        messageInput = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(200, -1), wxTE_PROCESS_ENTER);
        wxButton* sendButton = new wxButton(this, wxID_ANY, "Send");
        wxButton* sendPhotoButton = new wxButton(this, wxID_ANY, "Send Photo");

        inputSizer->Add(messageInput, 1, wxALL, 5);
        inputSizer->Add(sendButton, 0, wxALL, 5);
        inputSizer->Add(sendPhotoButton, 0, wxALL, 5);

        mainSizer->Add(inputSizer, 0, wxEXPAND | wxALL, 5);

        SetSizer(mainSizer);
        Layout();

        client = new ChatClient("127.0.0.1", 12345);

        if (client->Initialize() && client->Connect()) {
            client->Start(string(userName.mb_str()), chatDisplay);
            chatDisplay->AppendText("Connected to server\n");
        }
        else {
            chatDisplay->AppendText("Failed to connect to server\n");
        }

        messageInput->Bind(wxEVT_TEXT_ENTER, &ChatFrame::OnSendPressed, this);
        sendButton->Bind(wxEVT_BUTTON, &ChatFrame::OnSendPressed, this);
        sendPhotoButton->Bind(wxEVT_BUTTON, &ChatFrame::OnSendPhotoPressed, this);
    }

private:
    wxTextCtrl* chatDisplay;
    wxTextCtrl* messageInput;
    wxString userName;
    ChatClient* client;

    void OnSendPressed(wxCommandEvent& event) {
        SendMessage();
    }

    void SendMessage() {
        wxString message = messageInput->GetValue();
        if (!message.IsEmpty()) {
            chatDisplay->AppendText(userName + ": " + message + "\n");
            client->SendMessage(string(message.mb_str()));
            messageInput->Clear();
        }
    }

    void OnSendPhotoPressed(wxCommandEvent& event) {
        wxFileDialog openFileDialog(this, "Choose an image to send", "", "",
            "Image files (*.png;*.jpg;*.bmp)|*.png;*.jpg;*.bmp", wxFD_OPEN | wxFD_FILE_MUST_EXIST);

        if (openFileDialog.ShowModal() == wxID_OK) {
            wxString imagePath = openFileDialog.GetPath();
            client->SendPhoto(string(imagePath.mb_str()));
        }
    }
};

class ChatApp : public wxApp {
public:
    virtual bool OnInit() {
        wxTextEntryDialog nameDialog(nullptr, "Enter your name:", "User Name");
        if (nameDialog.ShowModal() == wxID_OK) {
            wxString userName = nameDialog.GetValue();

            ChatFrame* clientFrame = new ChatFrame("Client", wxColour(35, 35, 69), userName);  
            clientFrame->Show(true);
        }
        else {
            return false;
        }

        return true;
    }
};

wxIMPLEMENT_APP(ChatApp);

