#include "rake-c.h"

// Sends big endian uint32 header then message payload 
void send_msg(int sock, void *msg, uint32_t nbytes) {
    // big endian nbytes
    uint32_t be_nbytes = htonl(nbytes);
    send(sock, &be_nbytes, 4, 0);
    send(sock, msg, nbytes, 0);
}

// Reads big endian uint32 header, then message payload
void *recv_msg(int sock, int *size) {
    uint32_t *msg_len_ptr = calloc(1, 4);
    int nbytes = recv(sock, msg_len_ptr, 4, 0);
    if (nbytes == 0) return NULL;

    uint32_t msg_len = ntohl(*msg_len_ptr);
    *size = msg_len;
    void *msg = calloc(1, msg_len);
    recv(sock, msg, msg_len, 0);

    return msg;
}

int create_socket(char *hostname, char *port) {
    struct addrinfo hints, *res;
    int status;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;//AF_UNSPEC; // AF_INET or AF_INET6 to force version
    hints.ai_socktype = SOCK_STREAM;
    
    if ((status = getaddrinfo(hostname, port, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(1);
    }

    int s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    connect(s, res->ai_addr, res->ai_addrlen);
    return s;
}