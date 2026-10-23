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
#include <algorithm>

using namespace std;

int main(int argc, char* argv[]){

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
    udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sockfd < 0) {
        cerr << "ERROR creating UDP socket" << endl;
        return 1; 
    }

    sockaddr_in my_addr{};
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(my_udp_port);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(my_addr.sin_zero), '\0', 8);
    if (bind(udp_sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
        cerr << "ERROR binding UDP socket" << endl;
        close(udp_sockfd);
        return 1;
    }

    int tcp_sockfd;
    tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0);
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

    if (send(tcp_sockfd, msg, 6, 0) < 0){
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

    struct __attribute__((packed)) LspHeader {
        char origin_id;
        uint32_t seq_num;
        uint32_t neighbor_count;
    };

    struct __attribute__((packed)) LspLink {
        char neighbor_id;
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
    vector<NodeBlock> my_neighbors;

    fd_set readfds;

    while (true){
        FD_ZERO(&readfds);
        if (tcp_sockfd != -1) {
            FD_SET(tcp_sockfd, &readfds);
        }
        FD_SET(udp_sockfd, &readfds);

        int max_fd = (tcp_sockfd > udp_sockfd ? tcp_sockfd : udp_sockfd) + 1;

        struct timeval tv;
        tv.tv_sec = 10;
        tv.tv_usec = 0;

        int activity = select(max_fd, &readfds, NULL, NULL, &tv);

        if (activity < 0){
            cerr << "ERROR in select()" << endl;
            break;
        }

        if (activity == 0) {
            if (my_node_id != '\0') {
                cout << "\n--- Periodic Graph Update ---" << endl;
                
                vector<char> all_nodes;
                for (auto const& node_pair : network_graph) {
                    if (find(all_nodes.begin(), all_nodes.end(), node_pair.first) == all_nodes.end()) {
                        all_nodes.push_back(node_pair.first);
                    }
                    for (auto const& link_pair : node_pair.second) {
                        if (find(all_nodes.begin(), all_nodes.end(), link_pair.first) == all_nodes.end()) {
                            all_nodes.push_back(link_pair.first);
                        }
                    }
                }
                sort(all_nodes.begin(), all_nodes.end());

                for (size_t i = 0; i < all_nodes.size(); i++) {
                    char u = all_nodes[i];
                    cout << u << " -> ";
                    for (size_t j = 0; j < all_nodes.size(); j++) {
                        char v = all_nodes[j];
                        if (u == v) continue; 
                        
                        int cost = -1;
                        if (network_graph.count(u) && network_graph[u].count(v)) {
                            cost = network_graph[u][v];
                        } else if (network_graph.count(v) && network_graph[v].count(u)) {
                            cost = network_graph[v][u];
                        }
                        
                        cout << v << ":" << cost << " ";
                    }
                    cout << endl;
                }
                cout << "-----------------------------\n" << endl;

                if (!my_neighbors.empty()) {
                    char lsp_packet[1024]; 
                    int offset = 0;

                    LspHeader header = {my_node_id, htonl(my_seq_num), htonl(static_cast<uint32_t>(my_neighbors.size()))};
                    memcpy(lsp_packet, &header, sizeof(LspHeader));
                    offset += sizeof(LspHeader);

                    for (size_t i = 0; i < my_neighbors.size(); i++) {
                        LspLink link = {my_neighbors[i].id, my_neighbors[i].cost};
                        memcpy(lsp_packet + offset, &link, sizeof(LspLink));
                        offset += sizeof(LspLink);
                    }

                    for (size_t i = 0; i < my_neighbors.size(); i++) {
                        sockaddr_in dest_addr{};
                        dest_addr.sin_family = AF_INET;
                        dest_addr.sin_port = my_neighbors[i].port; 
                        dest_addr.sin_addr.s_addr = my_neighbors[i].ip; 
                        for (int k = 0; k < 2; k++) {
                            sendto(udp_sockfd, lsp_packet, offset, 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
                        }
                    }
                    my_seq_num++; 
                }
            }
            continue; 
        }

        if (tcp_sockfd != -1 && FD_ISSET(tcp_sockfd, &readfds)){
            uint32_t net_count = 0;
            int bytes_read = recv(tcp_sockfd, &net_count, sizeof(net_count), 0);
            
            if (bytes_read <= 0) {
                cout << "Oracle Node closed the TCP connection. Operating in UDP-only mode." << endl;
                close(tcp_sockfd);
                tcp_sockfd = -1;
                continue; 
            }

            uint32_t total_blocks = ntohl(net_count);
            vector<NodeBlock> all_blocks(total_blocks);
            
            int expected_bytes = total_blocks * sizeof(NodeBlock);
            int bytes_received_so_far = 0;
            char* buffer = (char*)all_blocks.data();

            while (bytes_received_so_far < expected_bytes) {
                int chunk = recv(tcp_sockfd, buffer + bytes_received_so_far, expected_bytes - bytes_received_so_far, 0);
                if (chunk <= 0) {
                    break;
                }
                bytes_received_so_far += chunk;
            }

            my_neighbors.clear();

            for (size_t i = 0; i < total_blocks; i++) {
                uint32_t actual_cost = ntohl(all_blocks[i].cost);
                if (actual_cost == 0) {
                    my_node_id = all_blocks[i].id;
                } else {
                    my_neighbors.push_back(all_blocks[i]);
                }
            }

            network_graph[my_node_id].clear();
            for (size_t i = 0; i < my_neighbors.size(); i++) {
                network_graph[my_node_id][my_neighbors[i].id] = ntohl(my_neighbors[i].cost);
            }

            cout << "\n--- Received Link State from Oracle Node ---" << endl;
            cout << my_node_id << "=0";
            for (size_t i = 0; i < my_neighbors.size(); i++) {
                cout << ", " << my_neighbors[i].id << "=" << ntohl(my_neighbors[i].cost);
            }
            cout << "\n--------------------------------------------\n" << endl;

            char lsp_packet[1024]; 
            int offset = 0;

            LspHeader header = {my_node_id, htonl(my_seq_num), htonl(static_cast<uint32_t>(my_neighbors.size()))};
            memcpy(lsp_packet, &header, sizeof(LspHeader));
            offset += sizeof(LspHeader);

            for (size_t i = 0; i < my_neighbors.size(); i++) {
                LspLink link = {my_neighbors[i].id, my_neighbors[i].cost};
                memcpy(lsp_packet + offset, &link, sizeof(LspLink));
                offset += sizeof(LspLink);
            }

            for (size_t i = 0; i < my_neighbors.size(); i++) {
                sockaddr_in dest_addr{};
                dest_addr.sin_family = AF_INET;
                dest_addr.sin_port = my_neighbors[i].port; 
                dest_addr.sin_addr.s_addr = my_neighbors[i].ip; 
                for (int k = 0; k < 2; k++) {
                    sendto(udp_sockfd, lsp_packet, offset, 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
                }
            }
            my_seq_num++;
        }

        if (FD_ISSET(udp_sockfd, &readfds)) {
            char udp_buffer[1024];
            sockaddr_in sender_addr;
            socklen_t sender_len = sizeof(sender_addr);
            
            int bytes_received = recvfrom(udp_sockfd, udp_buffer, sizeof(udp_buffer), 0, (struct sockaddr*)&sender_addr, &sender_len);
            
            if (bytes_received > 0) {
                LspHeader* incoming_header = (LspHeader*)udp_buffer;
                char origin = incoming_header->origin_id;
                uint32_t incoming_seq = ntohl(incoming_header->seq_num);
                uint32_t neighbor_count = ntohl(incoming_header->neighbor_count);

                if (latest_seq_nums.count(origin) == 0 || incoming_seq > latest_seq_nums[origin]) {
                    
                    latest_seq_nums[origin] = incoming_seq;
                    LspLink* incoming_links = (LspLink*)(udp_buffer + sizeof(LspHeader));
                    network_graph[origin].clear(); 
                    
                    for (size_t i = 0; i < neighbor_count; i++) {
                        uint32_t link_cost = ntohl(incoming_links[i].cost);
                        network_graph[origin][incoming_links[i].neighbor_id] = link_cost;
                    }

                    for (size_t i = 0; i < my_neighbors.size(); i++) {
                        sockaddr_in dest_addr{};
                        dest_addr.sin_family = AF_INET;
                        dest_addr.sin_port = my_neighbors[i].port;
                        dest_addr.sin_addr.s_addr = my_neighbors[i].ip;
                        for (int k = 0; k < 2; k++) {
                            sendto(udp_sockfd, udp_buffer, bytes_received, 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
                        }
                    }
                }
            }
        }
    }

    if (tcp_sockfd != -1) {
        close(tcp_sockfd);
    }
    close(udp_sockfd);
    return 0;
}