#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <jansson.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 6893               // Server port
#define MXCON 20                // Max concurrent connections
#define BBSZ 4096               // Request / response buffer size
#define SBSZ 256                // Message buffer size
#define DIV "\r\n\r\n"          // HTTP header separator
#define BDIR "/var/log/beacon/" // Directory for data storage

volatile int sfd;               // Server file descriptor

// Exit with error message
void die(int ret, char *msg) {
    if(msg[0]) fprintf(stderr, "%s\n", msg);
    exit(ret);
}

// Graceful shutdown on signal
static void sighandler(const int sig) {
    printf("Received signal, exiting\n");
    close(sfd);
    exit(0);
}

// Returns 1 if string looks like a valid IP address
static int isip(const char *str) {

    struct sockaddr_in s;
    return inet_pton(AF_INET, str, &(s.sin_addr));
}

// Sets appropriate resp status message based on stat
static void setmsg(const int stat, char *str) {
    if(stat == 0)
        strncpy(str, "OK\n", SBSZ);
    else if(stat == 1)
        strncpy(str, "Could not read data object\n", SBSZ);
    else if(stat == 2)
        strncpy(str, "Invalid IP address provided\n", SBSZ);
}

// Writes data to disk
static int wrfile(const char *host, const char *extip, const char *intip) {

    FILE *f;
    char *path = calloc(SBSZ, sizeof(char));

    snprintf(path, SBSZ, "%s%s", BDIR, host);
    f = fopen(path, "w");

    if(f != NULL) {
        char *str = calloc(BBSZ, sizeof(char));
        snprintf(str, BBSZ, "EXT: %s\nINT: %s\n", extip, intip);
        fputs(str, f);
        free(str);

    } else {
        fprintf(stderr, "ERROR: Could not open file for writing\n");
    }

    free(path);
    fclose(f);

    return 0;
}

// Handle client request
void *processreq(void *arg) {
    int cfd = *((int*)arg);
    char *buf = calloc(BBSZ, sizeof(char));
    char *msg = buf;
    ssize_t btr = recv(cfd, buf, BBSZ, 0);


    if(btr > 0) {
        int stat = 0;
        char *resp = calloc(BBSZ, sizeof(char));
        char *smsg = calloc(SBSZ, sizeof(char));

        json_t *json, *host, *extip, *intip;
        json_error_t err;

        msg = strstr(buf, DIV) + strlen(DIV);
        json = json_loads(msg, 0, &err);

        host = json_object_get(json, "host");
        extip = json_object_get(json, "extip");
        intip = json_object_get(json, "intip");

        if(!json_is_string(host) || !json_is_string(extip) || !json_is_string(intip))
            stat = 1;

        else if(!isip(json_string_value(extip)) || !isip(json_string_value(intip)))
            stat = 2;

        setmsg(stat, smsg);

        if(!stat) wrfile(json_string_value(host),
                         json_string_value(extip),
                         json_string_value(intip));

        snprintf(resp, BBSZ, "HTTP/1.1 200 OK\r\n"
                            "Content-Type: text/plain\r\n\r\n"
                            "Status: %d\n"
                            "Msg: %s\n", stat, smsg);

        int resplen = strlen(resp);

        send(cfd, resp, resplen, 0);
        free(smsg);
        free(resp);
    }

    close(cfd);
    free(arg);
    free(buf);

    return NULL;
}

int main(void) {
    struct sockaddr_in saddr;

    if((sfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        die(1, "ERROR: socket failure");

    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = INADDR_ANY;
    saddr.sin_port = htons(PORT);

    if(bind(sfd, (struct sockaddr*)&saddr, sizeof(saddr)) < 0)
        die(1, "ERROR: bind failure");

    if(listen(sfd, MXCON) < 0)
        die(1, "ERROR: listen failure");

    // signal(SIGCHLD, SIG_IGN);
    signal(SIGINT, sighandler);
    signal(SIGTERM, sighandler);

    // Server up and running - listening for connections forever
    for(;;) {
        struct sockaddr_in caddr;
        socklen_t caddrlen = sizeof(caddr);
        int *cfd = calloc(1, sizeof(int));

        if((*cfd = accept(sfd, (struct sockaddr*)&caddr, &caddrlen)) < 0) {
            fprintf(stderr, "ERROR: accept failure\n");

        } else {
            pthread_t tid;
            pthread_create(&tid, NULL, processreq, (void*)cfd);
            pthread_detach(tid);
        }
    }

    close(sfd);

    return 0;
}
