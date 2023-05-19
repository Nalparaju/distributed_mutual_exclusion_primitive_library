#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/select.h>

#include "network.h"
#include "mymutex.h"

#define MAX_HOSTS 10

#define max(a, b) ((a) > (b) ? (a) : (b))

pthread_mutex_t local_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t local_cond = PTHREAD_COND_INITIALIZER;

pthread_t thread;
Host *local_host = NULL;
unsigned int local_clock = 0;

// Flag to indicate if a reuest is currently being made
static bool is_requesting = false;
// Timestamp of the latest request made
static unsigned int request_timestamp = 0;
// Number of received replies to a request
static unsigned int num_replies = 0;
// Flag to Indicate that the critical section is in use
static bool in_critical_section = false;
// List of all hosts
static Host *hosts[MAX_HOSTS] = {NULL};
// List of Deferred Messages
static Message *deferred[MAX_HOSTS] = {NULL};
// Number of Hosts
static int num_hosts = 0;

// Main Thread function to handle receive_message
static void *mymutex_main(void *args) {
    fd_set fds;
    Host source;

    while (1) {
	// Waiting for Incoming Messages
        FD_ZERO(&fds);
        FD_SET(local_host->sockfd, &fds);

        select(local_host->sockfd + 1, &fds, NULL, NULL, NULL);

	// Process the received_message 
        if (FD_ISSET(local_host->sockfd, &fds)) {

            Message *msg = receive_message(&source);
            switch (msg->type)
            {
            case REQUEST:

		// Update the local clock with the maximum of its current value and the received timestamp.
                local_clock = max(local_clock, msg->timestamp) + 1;
                if (in_critical_section ||
                    (is_requesting && request_timestamp < msg->timestamp)) {
                    // Defer the reply to this request if in the critical section or if a request with a higher timestamp has already been made
                    for (int i = 0; i < MAX_HOSTS; i++) {
                        if (deferred[i] == NULL) {
                            deferred[i] = msg;
                            break;
                        }
                    }
                } else {
                    // Sending the reply for this request
                    send_message(REPLY, &source);
                }
                break;
            case REPLY:
		//updating the clock with the maximum of its  current value and the received timestamp
                local_clock = max(local_clock, msg->timestamp) + 1;
                if (is_requesting && ++num_replies == num_hosts - 1) {
                    // If we have received all replies num_hosts - 1, enter the critical section 
                    pthread_mutex_lock(&local_mutex);
                    in_critical_section = true;
                    is_requesting = false;
                    pthread_cond_signal(&local_cond);
                    pthread_mutex_unlock(&local_mutex);
                }
                break;
            default:
                break;
            }
        }
    }

    return NULL;
}

void mymutex_init(int local_hostid) {
    FILE *fp;
    char hostname[NI_MAXHOST];
    int id, port = PORT;
    Host *h;

// Check if the hosts file exists
    if (access(HOSTS_PATH, F_OK) == -1) {
        fprintf(stderr, "Could not find %s file\n", HOSTS_PATH);
        exit(EXIT_FAILURE);
    }

//Open the hosts file for reading
    if ((fp = fopen(HOSTS_PATH, "r")) == NULL) {
        fprintf(stderr, "Could not open %s file: %s\n", HOSTS_PATH, strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Read the hosts file
    while (fscanf(fp, "%d %s", &id, hostname) == 2 && num_hosts++ < MAX_HOSTS) {
        if (id == local_hostid) {
            h = new_host(hostname, port, true);
            local_host = h;
        } else {
            h = new_host(hostname, port, false);
            for (int i = 0; i < MAX_HOSTS; i++) {
                if (hosts[i] == NULL) {
                    hosts[i] = h;
                    break;
                }
            }
        }

        h->hostid = id;
        h->port = port;
        strncpy(h->hostname, hostname, NI_MAXHOST);
    }

    fclose(fp);

// Start the thread to handle message receiving and processing
    pthread_create(&thread, NULL, mymutex_main, NULL);
}

void mymutex_acquire_lock(void) {
// Check if already in the critical section
    if (in_critical_section) {
        fprintf(stderr, "Error: Already in critical section\n");
        return;
    }
//Set the Flag to indicate a request is being made and update the request timestamp
    is_requesting = true;
    request_timestamp = local_clock;
    // Broadcast the request message to all hosts
    broadcast_message(REQUEST, hosts, num_hosts);

    pthread_mutex_lock(&local_mutex);
    while(is_requesting)
	// Waiting for the access to enter critical section
        pthread_cond_wait(&local_cond, &local_mutex);
}

// Releasing the critical section lock
void mymutex_release_lock(void) {
    // Check if not in the critical section
    if (!in_critical_section) {
        fprintf(stderr, "Error: Not in critical section\n");
        return;
    }

    // Reset the flags and counters

    is_requesting = false;
    in_critical_section = false;
    num_replies = 0;

    // Reply to all hosts in the deferred list
    for (int i = 0; i < num_hosts; i++) {
        if (deferred[i] != NULL) {
            for (int j = 0; j < num_hosts; j++) {
                if (hosts[j] && hosts[j]->hostid == deferred[i]->sender_id) {
                    send_message(REPLY, hosts[j]);
                    break;
                }
            }
            free(deferred[i]);
            deferred[i] = NULL;
            sleep(1);
        }
    }

    pthread_mutex_unlock(&local_mutex);
}

void mymutex_destroy(void) {
    // Freeing allocated memory for hosts and deferred messages
    for (int i = 0; i < MAX_HOSTS; i++) {
        if (hosts[i] != NULL)
            free(hosts[i]);
        if (deferred[i] != NULL)
            free(deferred[i]);
    }
    // Freeing memory for the host
    free(local_host);
}
