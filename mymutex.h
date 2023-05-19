#ifndef MYMUTEX_H
#define MYMUTEX_H

#ifndef MAX_PATHNAME_LEN
#define MAX_PATHNAME_LEN 1024
#endif

#define err_exit(msg) do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define HOSTS_PATH "./processes.hosts"
#define PORT 9000

void mymutex_init(int local_hostid);
void mymutex_acquire_lock(void);
void mymutex_release_lock(void);
void mymutex_destroy(void);

#endif