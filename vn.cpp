#include <iostream>
#include <string.h>
#include <vector>
#include <map>
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

    struct __attribute__((packed)) NodeBlock {
        char id;
        uint32_t ip;
        uint16_t port;
        uint32_t cost;
    };

    char my_node_id = '\0'; 

    struct PeerAddress {
        uint32_t ip;
        uint16_t port;
    };

    std::map<char, PeerAddress> direct_neighbors;
    std::map<char, std::map<char, uint32_t>> network_graph;
    std::map<char, uint32_t> latest_seq_nums;
    uint32_t my_seq_num = 1;

    fd_set readfds;
    int max_fd = tcp_sockfd + 1;

    while (true){
        FD_ZERO(&readfds);
        FD_SET(tcp_sockfd, &readfds);

        if(select(max_fd,&readfds,NULL,NULL,NULL) < 0){
            cerr << "ERROR in select()" << endl;
            break;
        }

        if (FD_ISSET(tcp_sockfd, &readfds)){
            uint32_t net_count = 0;
            int bytes_read = recv(tcp_sockfd, &net_count, sizeof(net_count), 0);
            
            if (bytes_read <= 0) {
                cout << "Oracle Node closed the connection." << endl;
                break;
            }

            uint32_t total_blocks = ntohl(net_count);
            vector<NodeBlock> all_blocks(total_blocks);
            
            int expected_bytes = total_blocks * sizeof(NodeBlock);
            int bytes_received_so_far = 0;
            char* buffer = (char*)all_blocks.data();

            while (bytes_received_so_far < expected_bytes) {
                int chunk = recv(tcp_sockfd, 
                                 buffer + bytes_received_so_far, 
                                 expected_bytes - bytes_received_so_far, 
                                 0);
                
                if (chunk <= 0) {
                    cerr << "ERROR: Connection dropped during payload transfer." << endl;
                    break;
                }
                bytes_received_so_far += chunk;
            }

            char my_id;
            vector<NodeBlock> my_neighbors;

            cout << "\n--- Received Link State from Oracle Node ---" << endl;
            
            for (int i = 0; i < total_blocks; i++) {
                uint32_t actual_cost = ntohl(all_blocks[i].cost);
                
                if (actual_cost == 0) {
                    my_id = all_blocks[i].id;
                    cout << "Assigned Identity: Node " << my_id << endl;
                } else {
                    my_neighbors.push_back(all_blocks[i]);
                    cout << "Neighbor: Node " << all_blocks[i].id 
                         << " | Link Cost: " << actual_cost << endl;
                }
            }
            cout << "--------------------------------------------\n" << endl;
        }
    }
    
    
    
    return 0;
}