#ifndef _NETWORK_H_
#define _NETWORK_H_

#include <sys/types.h>
#include <arpa/inet.h>
#include <stdbool.h>

#ifndef NI_MAXHOST
#define NI_MAXHOST 1025
#endif

#define err_exit(msg) do { perror(msg); exit(EXIT_FAILURE); } while (0)

typedef enum {
    REQUEST,
    REPLY
} MessageType;

typedef struct {
    MessageType type;
    unsigned int sender_id;
    char sender[NI_MAXHOST];
    unsigned int timestamp;
} Message;

typedef struct {
    int sockfd;
    int hostid;
    int port;
    char hostname[NI_MAXHOST];
    struct sockaddr_in addr;
    socklen_t addr_len;
} Host;

Host *new_host(const char *hostname, int port, bool sockbind);
void send_message(MessageType msg_type, Host *dest);
void broadcast_message(MessageType msg_type, Host **hosts, int num_hosts);
Message *receive_message(Host *src);

#endif