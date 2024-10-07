#include "TCPRequestChannel.h"

using namespace std;


TCPRequestChannel::TCPRequestChannel (const std::string _ip_address, const std::string _port_no) {
    
    if(_ip_address == ""){ //if it is requesting from server, it passes in empty string
        struct sockaddr_in server;
        int server_sock;


        //make a socket for server, IPv4, TCP, 0 protocal
        server_sock = socket(AF_INET, SOCK_STREAM, 0);
        //set the server sock to this chan's socket fd
        sockfd = server_sock;
        if(server_sock == -1 ){
            perror("Socket Failed");
            exit(EXIT_FAILURE);
        }

        //set up the server variable
        server.sin_family = AF_INET; //set family to IPv4
        server.sin_addr.s_addr = INADDR_ANY; //server accpets any kind of IP address
        server.sin_port = htons(stoi(_port_no)); //set the port number

        //binding the server
        if(bind(server_sock, (const struct sockaddr*) &server, sizeof(server)) == -1){
            perror("Bind Failed");
            close(server_sock);
            exit(EXIT_FAILURE);
        }

        //listen from the socket, wait for items
        if(listen(server_sock, 5) < 0){
            perror("Listen Failed");
            close(server_sock);
            exit(EXIT_FAILURE);
        }
        
        //we accept each connection after creating the server...
    }
    else{ //if it is requesting from client, it passes in an IP Address
        //set a server_info variable to let the client connects to the IP Address(server host)
        struct sockaddr_in server_info;
        int client_sock;

        //socket for client
        client_sock = socket(AF_INET, SOCK_STREAM, 0);
        sockfd = client_sock;
        
        //generate server info
        server_info.sin_family = AF_INET; //set IPv4
        server_info.sin_port = htons(stoi(_port_no)); //set the port number
        if(inet_pton(AF_INET, _ip_address.c_str(), &server_info.sin_addr) <= 0){ //turn the IP Address thats in string to a numeric IP address presentation
            perror("Invalid Address");
            close(client_sock);
            exit(EXIT_FAILURE);
        }

        //connect the client socket to the IP Address (server)
        if(connect(client_sock, (const struct sockaddr*) &server_info, sizeof(server_info)) < 0){
            perror("Connection Failed");
        }

    }
}

TCPRequestChannel::TCPRequestChannel (int _sockfd) {
    //set the given _sockfd to this sockfd
    sockfd = _sockfd;
}

TCPRequestChannel::~TCPRequestChannel () {
    //close the socket
    close(this->sockfd);
}

int TCPRequestChannel::accept_conn () {
    //declare accepted file discriptor
    int acceptfd;

    //variable to strore the accepted IP Address
    struct sockaddr_in accpet_addr;
    //delcare a address length variable to store the address length 
    socklen_t addr_len = sizeof(accpet_addr);

    //accept connection from server
    acceptfd = accept(sockfd, (struct sockaddr*) &accpet_addr, &addr_len);

    //return the file discriptor of the client
    return acceptfd;
}

int TCPRequestChannel::cread (void* msgbuf, int msgsize) {
    ssize_t num_bytes;
    //read from socket
    num_bytes = read(sockfd, msgbuf, (size_t)msgsize);
    if(num_bytes == -1){
        perror("Read Error");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    //return the number of bytes read
    return num_bytes;
}

int TCPRequestChannel::cwrite (void* msgbuf, int msgsize) {
    ssize_t num_bytes;
    //wrtie to socket
    num_bytes = write(sockfd, msgbuf, (size_t)msgsize);
    if(num_bytes == -1){
        perror("Write Error");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    return num_bytes;
}
