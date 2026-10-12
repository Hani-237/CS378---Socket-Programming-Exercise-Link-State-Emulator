#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

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

    int sockfd;
    sockfd = socket(PF_INET,SOCK_DGRAM,0);
    if (sockfd < 0) {
        cerr << "ERROR creating UDP socket" << endl;
        return 1; 
    }

    sockaddr_in my_addr{};
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(my_udp_port);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(my_addr.sin_zero),'\0',8);
    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
        cerr << "ERROR binding UDP socket" << endl;
        close(sockfd);
        return 1;
    }


    return 0;
}