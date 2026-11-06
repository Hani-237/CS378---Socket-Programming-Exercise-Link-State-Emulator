
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

using namespace std;

int main() {

    // 1. Create TCP socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        perror("Socket creation failed");
        return 1;
    }

    cout << "Socket created successfully!" << endl;

    // 2. Specify the server address (H3 - MacBook)
    sockaddr_in their_addr{};

    their_addr.sin_family = AF_INET;
    their_addr.sin_port = htons(2300);

    // Server IP address
    if (inet_pton(AF_INET, "192.168.1.106",
                  &their_addr.sin_addr) <= 0) {
        perror("Invalid server IP address");
        close(sockfd);
        return 1;
    }

    // 3. Connect to the server
    if (connect(sockfd, (struct sockaddr *)&their_addr,
                sizeof(their_addr)) < 0) {
        perror("Connection failed");
        close(sockfd);
        return 1;
    }

    cout << "Connected to server successfully!" << endl;

    // 4. Send message to server
    const char *msg = "Hi, This is Client!";

    int len = strlen(msg);

    int bytes_sent = send(sockfd, msg, len, 0);

    if (bytes_sent < 0) {
        perror("Send failed");
        close(sockfd);
        return 1;
    }

    cout << "Bytes sent: " << bytes_sent << endl;

    // 5. Receive message from server
    char buffer[1000] = {};

    int bytes_received = recv(sockfd, buffer,
                              sizeof(buffer) - 1, 0);

    if (bytes_received < 0) {
        perror("Receive failed");
        close(sockfd);
        return 1;
    }

    if (bytes_received == 0) {
        cout << "Server closed the connection." << endl;
    } else {
        buffer[bytes_received] = '\0';

        cout << "Message from server: "
             << buffer << endl;

        cout << "Bytes received: "
             << bytes_received << endl;
    }

    // 6. Close socket
    close(sockfd);

    cout << "Connection closed." << endl;

    return 0;
}
