// server.c
#include <stdio.h>
#include <winsock2.h>
#include <process.h>

#define MAX_CLIENTS 3
#define MAX_MESSAGE_SIZE 100

typedef struct {
    SOCKET socket;
    int clientId;
    char username[MAX_MESSAGE_SIZE];
} ClientInfo;

typedef struct {
    int senderId;
    int recipientId;
    char message[MAX_MESSAGE_SIZE];
} PrivateMessage;

ClientInfo clients[MAX_CLIENTS];

// Private mesaj gönderme iþlevi
void ProcessPrivateMessage(PrivateMessage* privateMessage, ClientInfo clients[], int numClients) {
    int senderId = privateMessage->senderId;
    int recipientId = privateMessage->recipientId;

    // Hedef client'ý bul
    if (recipientId >= 1 && recipientId <= numClients) {
        // Private mesajý ilgili client'a gönder
        char msg[MAX_MESSAGE_SIZE];
        snprintf(msg, sizeof(msg), "Private Message from Client %d: %s", senderId, privateMessage->message);
        send(clients[recipientId - 1].socket, msg, strlen(msg), 0);
        printf("Private Message sent to Client %d\n", recipientId);
    } else {
        printf("Invalid recipient ID %d for private message from Client %d\n", recipientId, senderId);
    }
}

// Client için thread iþlevi
unsigned int __stdcall ClientThread(void* clientInfoPtr) {
    ClientInfo* clientInfo = (ClientInfo*)clientInfoPtr;
    SOCKET clientSocket = clientInfo->socket;
    int clientId = clientInfo->clientId;

    char message[MAX_MESSAGE_SIZE];

    while (1) {
        memset(message, 0, sizeof(message));
        int bytesReceived = recv(clientSocket, message, MAX_MESSAGE_SIZE, 0);
        if (bytesReceived <= 0) {
            printf("Client %s disconnected\n", clientInfo->username);
            break;
        }

        if (strncmp(message, "\\pm", 3) == 0) {
            PrivateMessage privateMessage;
            sscanf(message, "\\pm %d %[^\n]", &privateMessage.recipientId, privateMessage.message);
            privateMessage.senderId = clientId;

            // Private mesajý iþle
            ProcessPrivateMessage(&privateMessage, clients, MAX_CLIENTS);
        } else {
            printf("Client %s says: %s\n", clientInfo->username, message);
        }
    }

    closesocket(clientSocket);
    _endthreadex(0);
    return 0;
}

int main() {
    WSADATA wsaData;
    SOCKET serverSocket, clientSockets[MAX_CLIENTS];
    struct sockaddr_in serverAddr, clientAddr;
    int i;

    // Winsock baþlatýlýyor
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        perror("WSAStartup failed");
        return 1;
    }

    // Socket oluþturuluyor
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        perror("Socket creation failed");
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8096);

    // Socket'i baðlama
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        perror("Bind failed");
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    // Dinlemeye baþla
    if (listen(serverSocket, MAX_CLIENTS) == SOCKET_ERROR) {
        perror("Listen failed");
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    // Maksimum client sayýsý kadar baðlantý kabul et
    for (i = 0; i < MAX_CLIENTS; ++i) {
        int addrLen = sizeof(clientAddr);
        clientSockets[i] = accept(serverSocket, (struct sockaddr*)&clientAddr, &addrLen);
        if (clientSockets[i] == INVALID_SOCKET) {
            perror("Accept failed");
            closesocket(serverSocket);
            WSACleanup();
            return 1;
        }

        printf("Enter a username for client %d: ", i + 1);
        fgets(clients[i].username, MAX_MESSAGE_SIZE, stdin);
        clients[i].username[strcspn(clients[i].username, "\n")] = '\0';

        clients[i].socket = clientSockets[i];
        clients[i].clientId = i + 1;

        printf("Client %s connected\n", clients[i].username);

        // Client için yeni bir thread baþlatýlýyor
        ClientInfo* clientInfo = (ClientInfo*)malloc(sizeof(ClientInfo));
        *clientInfo = clients[i];

        HANDLE threadHandle = (HANDLE)_beginthreadex(NULL, 0, ClientThread, clientInfo, 0, NULL);
        if (threadHandle == NULL) {
            perror("Thread creation failed");
            closesocket(serverSocket);
            WSACleanup();
            return 1;
        }

        CloseHandle(threadHandle);
    }

    // Sunucu durdurulmadýðý sürece çalýþmaya devam eder
    while (1) {
        Sleep(1000);
    }

    // Client baðlantýlarý kapatýlýyor
    for (i = 0; i < MAX_CLIENTS; ++i) {
        closesocket(clientSockets[i]);
    }
    closesocket(serverSocket);
    WSACleanup();

    return 0;
}
