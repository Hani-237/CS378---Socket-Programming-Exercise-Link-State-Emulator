#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

using namespace std;

int main(){
    int sockfd;
    sockfd = socket(PF_INET,SOCK_STREAM,0);

    sockaddr_in my_addr{};
    sockaddr_in their_addr{};
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(2300);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(my_addr.sin_zero),'\0',8);

    bind(sockfd , (struct sockaddr *)&my_addr , sizeof(struct sockaddr));
    listen(sockfd,10);

    socklen_t addr_len = sizeof(their_addr);
    int new_fd = accept(sockfd, (struct sockaddr *)&their_addr , &addr_len);

    char *msg = "Hi, This is Server!";
    int len = strlen(msg);
    int bytes_sent = send(new_fd,msg,len,0);
    cout << bytes_sent << endl;

    char buffer[1000] = {};
    recv(new_fd,buffer,sizeof(buffer),0);

    cout << "Recieved Message : " << buffer << endl;

    close(sockfd);
    close(new_fd);

    return 0;

}