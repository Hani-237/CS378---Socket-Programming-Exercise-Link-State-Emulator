#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>

using namespace std;

int main(int argc,char* argv[]){

    if(argc != 4){
        cerr << "ERROR : Invalid arguments" << endl;
        return 1;
    }

    string on_ip = argv[1];
    int on_port = stoi(argv[2]);
    int my_udp_port = stoi(argv[3]);

    if(my_udp_port <= 1024 || my_udp_port > 65535){
        cerr << "ERROR : Invalid port usage, use ports in the range [1025,65535]" << endl;
        return 1;
    }
    if(on_port < 0 || on_port > 65535){
        cerr << "ERROR : Invalid port number of Oracle Node" << endl;
        return 1;
    }

    int udp_sockfd;
    udp_sockfd = socket(AF_INET,SOCK_DGRAM,0);
    if (udp_sockfd < 0) {
        cerr << "ERROR creating UDP socket" << endl;
        return 1; 
    }

    sockaddr_in my_addr{};
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(my_udp_port);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(my_addr.sin_zero),'\0',8);
    if (bind(udp_sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
        cerr << "ERROR binding UDP socket" << endl;
        close(udp_sockfd);
        return 1;
    }

    int tcp_sockfd;
    tcp_sockfd = socket(AF_INET,SOCK_STREAM,0);
    if (tcp_sockfd < 0) {
        cerr << "ERROR creating TCP socket" << endl;
        return 1; 
    }

    sockaddr_in on_addr{};
    on_addr.sin_family = AF_INET;
    on_addr.sin_port = htons(on_port);
    on_addr.sin_addr.s_addr = inet_addr(on_ip.c_str());
    if (on_addr.sin_addr.s_addr == INADDR_NONE) {
        cerr << "ERROR: Invalid Oracle Node IP address " << endl;
        close(tcp_sockfd);
        close(udp_sockfd);
        return 1;
    }
    
    if (connect(tcp_sockfd, (struct sockaddr *)&on_addr, sizeof(on_addr)) < 0) {
        cerr << "ERROR connecting to Oracle Node" << endl;
        close(tcp_sockfd);
        close(udp_sockfd);
        return 1;
    }

    char my_hostname[256];
    if (gethostname(my_hostname, sizeof(my_hostname)) < 0) {
        cerr << "ERROR: gethostname failed" << endl;
        close(tcp_sockfd);
        close(udp_sockfd);
        return 1;
    }

    struct hostent *h = gethostbyname(my_hostname);
    if (h == NULL) {
        cerr << "ERROR: gethostbyname failed" << endl;
        close(tcp_sockfd);
        close(udp_sockfd);
        return 1;
    }

    char msg[6];
    memcpy(msg, h->h_addr, 4);
    uint16_t vn_port = htons(my_udp_port);
    memcpy(msg + 4, &vn_port, 2);

    if (send(tcp_sockfd,msg,6,0) < 0){
        cerr << "ERROR sending CONNECT message to ON" << endl;
        close(tcp_sockfd);
        close(udp_sockfd);
        return 1;
    }

    
    
    
    return 0;
}