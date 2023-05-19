#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <netdb.h>
#include <sys/socket.h>

#include "network.h"

extern Host *local_host;
extern unsigned int local_clock;

Host *new_host(const char *hostname, int port, bool sockbind) {
    Host *new_host;
    struct hostent *host;

    if (!(new_host = malloc(sizeof(Host))))
        err_exit("malloc");

    memset(&new_host->addr, 0, sizeof(struct sockaddr_in));
    new_host->addr.sin_family = AF_INET;
    new_host->addr.sin_port = htons(port);
    new_host->addr_len = sizeof(struct sockaddr);

    // Get the host address from the hostname
    if ((host = gethostbyname(hostname)) == NULL)
        err_exit(hostname);
    memcpy(&new_host->addr.sin_addr, host->h_addr_list[0], host->h_length);

    if (sockbind) {
        if ((new_host->sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
            err_exit("socket");

        // Reuse the socket if it's already in use
        int optval = 1;
        if (setsockopt(new_host->sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
            err_exit("setsockopt");

        if (bind(new_host->sockfd, (struct sockaddr *)&new_host->addr, new_host->addr_len) == -1)
            err_exit("bind");
    }

    return new_host;
}

void send_message(MessageType msg_type, Host *dest) {
    Message msg;

    msg.type = msg_type;
    msg.sender_id = local_host->hostid;
    strncpy(msg.sender, local_host->hostname, NI_MAXHOST);
    msg.timestamp = ++local_clock;

    if (sendto(local_host->sockfd, &msg, sizeof(Message), 0,
               (struct sockaddr *)&dest->addr, dest->addr_len) == -1)
        err_exit("sendto");
}

//Broadcast the message to a list of hosts.

void broadcast_message(MessageType msg_type, Host **hosts, int num_hosts) {
    for (int i = 0; i < num_hosts; i++) {
        if (hosts[i] && hosts[i]->hostid != local_host->hostid)
            send_message(msg_type, hosts[i]);
    }
}


// Receive_message from a host.
Message *receive_message(Host *src) {
    Message *msg;

    if (!(msg = malloc(sizeof(Message))))
        err_exit("malloc");

    if (src) {
        src->addr_len = sizeof(struct sockaddr);
        if (recvfrom(local_host->sockfd, msg, sizeof(Message), 0,
                     (struct sockaddr *)&src->addr, &src->addr_len) == -1)
            err_exit("recvfrom");
        strncpy(src->hostname, msg->sender, NI_MAXHOST);
        src->hostid = msg->sender_id;
        src->port = ntohs(src->addr.sin_port);
        src->sockfd = local_host->sockfd;
    }else{
        if(recvfrom(local_host->sockfd, msg, sizeof(Message), 0, NULL, NULL) == -1)
            err_exit("recvfrom");
    }
    return msg;
}
