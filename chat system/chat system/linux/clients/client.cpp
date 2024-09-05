// Client.cpp
#include <wx/wx.h>
#include <wx/sizer.h>
#include <wx/textdlg.h>
#include <wx/filedlg.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <string>
#include <fstream>
#include <vector>
#include <iostream>

using namespace std;
class ChatClient
{
public:
    ChatClient(const  string &serverAddress, int port)
        : serverAddress(serverAddress), port(port) {}

    bool Initialize()
    {
        return true; // No initialization required for Linux sockets
    }

    bool Connect()
    {
        s = socket(AF_INET, SOCK_STREAM, 0);
        if (s == -1)
        {
            return false;
        }

        sockaddr_in serveraddr;
        serveraddr.sin_family = AF_INET;
        serveraddr.sin_port = htons(port);
        inet_pton(AF_INET, serverAddress.c_str(), &(serveraddr.sin_addr));

        if (connect(s, reinterpret_cast<sockaddr *>(&serveraddr), sizeof(serveraddr)) == -1)
        {
            close(s);
            return false;
        }

        return true;
    }

    void Start(const  string &name, wxTextCtrl *output)
    {
        this->name = name;
        this->output = output;

         thread ReceiverThread(&ChatClient::ReceiveMessage, this);
        ReceiverThread.detach();
    }

    void SendMessage(const  string &message)
    {
         string msg = name + ": " + message;

        char header = 1; // Text message header
        send(s, &header, sizeof(header), 0);
        send(s, msg.c_str(), msg.length(), 0);
    }

    void SendPhoto(const  string &imagePath)
    {
         ifstream imageFile(imagePath,  ios::binary);
        if (!imageFile.is_open())
        {
            output->AppendText("Failed to open the image file!\n");
            return;
        }

         vector<char> imageData(( istreambuf_iterator<char>(imageFile)),  istreambuf_iterator<char>());
        imageFile.close();

        char header = 2; // Image header
        send(s, &header, sizeof(header), 0);

        int imgSize = imageData.size();
        send(s, reinterpret_cast<char *>(&imgSize), sizeof(imgSize), 0);

        send(s, imageData.data(), imgSize, 0);

        output->AppendText("Image sent successfully!\n");
    }

private:
    int s;
     string serverAddress;
    int port;
     string name;
    wxTextCtrl *output;

    void ReceiveMessage()
    {
        char buffer[4096];
        while (true)
        {
            char header;
            int bytesReceived = recv(s, &header, sizeof(header), 0);
            if (bytesReceived <= 0)
            {
                output->AppendText("Disconnected\n");
                break;
            }

            if (header == 1)
            {
                int recvLength = recv(s, buffer, sizeof(buffer), 0);
                if (recvLength > 0)
                {
                     string msg(buffer, recvLength);
                    wxString wxMsg = wxString::FromUTF8(msg.c_str());
                    output->AppendText(wxMsg + "\n");
                }
            }
            else if (header == 2)
            {
                int imgSize;
                recv(s, reinterpret_cast<char *>(&imgSize), sizeof(imgSize), 0);

                 vector<char> imgData(imgSize);
                int totalBytesReceived = 0;
                while (totalBytesReceived < imgSize)
                {
                    bytesReceived = recv(s, imgData.data() + totalBytesReceived, imgSize - totalBytesReceived, 0);
                    if (bytesReceived <= 0)
                    {
                        output->AppendText("Disconnected during image transfer\n");
                        break;
                    }
                    totalBytesReceived += bytesReceived;
                }

                 ofstream outputImage("received_image.jpg",  ios::binary);
                outputImage.write(imgData.data(), imgSize);
                outputImage.close();

                output->AppendText("Image received and saved as 'received_image.jpg'\n");
            }
        }

        close(s);
    }
};

class ChatFrame : public wxFrame
{
public:
    ChatFrame(const wxString &title, const wxColour &bgColor, const wxString &userName)
        : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(400, 600)),
          chatDisplay(nullptr), messageInput(nullptr), userName(userName)
    {

        SetBackgroundColour(bgColor);

        wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);

        wxStaticText *header = new wxStaticText(this, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
        header->SetBackgroundColour(*wxBLACK);
        mainSizer->Add(header, 0, wxEXPAND | wxALL, 10);

        chatDisplay = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
        mainSizer->Add(chatDisplay, 1, wxEXPAND | wxALL, 10);

        wxBoxSizer *bottomSizer = new wxBoxSizer(wxHORIZONTAL);

        messageInput = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
        bottomSizer->Add(messageInput, 1, wxEXPAND | wxALL, 10);

        wxButton *sendButton = new wxButton(this, wxID_ANY, "Send");
        bottomSizer->Add(sendButton, 0, wxEXPAND | wxALL, 10);

        wxButton *sendImageButton = new wxButton(this, wxID_ANY, "Send Image");
        bottomSizer->Add(sendImageButton, 0, wxEXPAND | wxALL, 10);

        mainSizer->Add(bottomSizer, 0, wxEXPAND | wxALL, 10);

        SetSizer(mainSizer);

        Bind(wxEVT_BUTTON, &ChatFrame::OnSendMessage, this, sendButton->GetId());
        Bind(wxEVT_TEXT_ENTER, &ChatFrame::OnSendMessage, this, messageInput->GetId());
        Bind(wxEVT_BUTTON, &ChatFrame::OnSendImage, this, sendImageButton->GetId());

        //for locallay coonect to "127.0.0.1" else go to see IPv4 address of Wifi
       //chatClient = new ChatClient("192.168.1.215", 12345);
       chatClient = new ChatClient("127.0.0.1", 12345);
        // chatClient = new ChatClient("192.168.1.95", 12345);
        if (!chatClient->Initialize() || !chatClient->Connect())
        {
            chatDisplay->AppendText("Failed to connect to the server.\n");
        }
        else
        {
            chatClient->Start(userName.ToStdString(), chatDisplay);
            chatDisplay->AppendText("Connected to the server.\n");
        }
    }

    ~ChatFrame()
    {
        delete chatClient;
    }

private:
    wxTextCtrl *chatDisplay;
    wxTextCtrl *messageInput;
    wxString userName;
    ChatClient *chatClient;

    void OnSendMessage(wxCommandEvent &event)
    {
        wxString message = messageInput->GetValue();
        if (!message.IsEmpty())
        {
            chatClient->SendMessage(message.ToStdString());
            messageInput->Clear();
        }
    }

    void OnSendImage(wxCommandEvent &event)
    {
        wxFileDialog openFileDialog(this, _("Open Image file"), "", "", "Image files (*.png;*.jpg;*.jpeg)|*.png;*.jpg;*.jpeg", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
        if (openFileDialog.ShowModal() == wxID_CANCEL)
            return;

        wxString filePath = openFileDialog.GetPath();
        chatClient->SendPhoto(filePath.ToStdString());
    }
};

class ChatApp : public wxApp
{
public:
    virtual bool OnInit()
    {
        wxColour bgColor = *wxCYAN;
        wxString userName = wxGetTextFromUser("Enter your username:", "Username", "User");
        ChatFrame *frame = new ChatFrame("LETS CHAT", bgColor, userName);
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(ChatApp);
