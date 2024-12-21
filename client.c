// client1.c
#include <stdio.h>
#include <winsock2.h>
#include <string.h>

#define MAX_MESSAGE_SIZE 100

// ClientInfo yapısını tanımlıyoruz
typedef struct {
    SOCKET socket;
    int clientId;
    char username[20];
} ClientInfo;

// PrivateMessage yapısını tanımlıyoruz
typedef struct {
    int recipientId;
    char message[MAX_MESSAGE_SIZE];
} PrivateMessage;

// Private mesajları işle
void ProcessPrivateMessage(PrivateMessage* privateMessage) {
    printf("Private Message from Client %d: %s\n", privateMessage->recipientId, privateMessage->message);
}

// Client için thread işlevi
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
            ProcessPrivateMessage(&privateMessage);
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
    SOCKET clientSocket;
    struct sockaddr_in serverAddr;

    // Winsock başlatılıyor
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        perror("WSAStartup failed");
        return 1;
    }

    // Socket oluşturuluyor
    if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        perror("Socket creation failed");
        return 1;
    }

    // Server adresi belirleniyor
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port = htons(8096);

    // Server'a bağlanma
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        perror("Connection failed");
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    // Client bilgileri
    ClientInfo clientInfo;
    clientInfo.socket = clientSocket;
    clientInfo.clientId = 1;  // Örneğin 1. client olarak başlıyoruz
    strcpy(clientInfo.username, "Client1");

    // Client thread'i başlatılıyor
    HANDLE threadHandle = (HANDLE)_beginthreadex(NULL, 0, ClientThread, &clientInfo, 0, NULL);
    if (threadHandle == NULL) {
        perror("Thread creation failed");
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    char message[MAX_MESSAGE_SIZE];
    while (1) {
        printf("Enter a message: ");
        fgets(message, MAX_MESSAGE_SIZE, stdin);

        if (strncmp(message, "\\pm", 3) == 0) {
            int sentBytes = send(clientSocket, message, strlen(message), 0);

            if (sentBytes == SOCKET_ERROR) {
                perror("Error sending message");
            } else {
                printf("Message sent successfully\n");
            }
        } else {
            int sentBytes = send(clientSocket, message, strlen(message), 0);

            if (sentBytes == SOCKET_ERROR) {
                perror("Error sending message");
            } else {
                printf("Message sent successfully\n");
            }
        }
    }

    // Thread kapanışı ve soket temizliği
    CloseHandle(threadHandle);
    closesocket(clientSocket);
    WSACleanup();

    return 0;
}
