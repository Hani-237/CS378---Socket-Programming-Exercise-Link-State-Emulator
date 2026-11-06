
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

using namespace std;

int main() {

    // 1. Create UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0) {
        perror("Socket creation failed");
        return 1;
    }

    cout << "UDP socket created successfully!" << endl;

    // 2. Configure server address
    sockaddr_in server_addr{};

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8081);

    if (inet_pton(AF_INET, "192.168.1.106",
                  &server_addr.sin_addr) <= 0) {
        perror("Invalid server IP");
        close(sockfd);
        return 1;
    }

    // 3. Send message to server
    const char *msg = "Hi, This is UDP Client!";

    int bytes_sent = sendto(
        sockfd,
        msg,
        strlen(msg),
        0,
        (struct sockaddr *)&server_addr,
        sizeof(server_addr)
    );

    if (bytes_sent < 0) {
        perror("Send failed");
        close(sockfd);
        return 1;
    }

    cout << "Message sent: " << bytes_sent << " bytes" << endl;

    // 4. Receive server response
    char buffer[1000] = {};

    sockaddr_in from_addr{};
    socklen_t from_len = sizeof(from_addr);

    int bytes_received = recvfrom(
        sockfd,
        buffer,
        sizeof(buffer) - 1,
        0,
        (struct sockaddr *)&from_addr,
        &from_len
    );

    if (bytes_received < 0) {
        perror("Receive failed");
        close(sockfd);
        return 1;
    }

    buffer[bytes_received] = '\0';

    cout << "Message from server: " << buffer << endl;

    // 5. Close socket
    close(sockfd);

    cout << "UDP communication completed!" << endl;

    return 0;
}
