#include <cerrno>

#define DATA_SIZE 4096
typedef struct connection_list{
    int client_socket, remote_socket;
    int data_toremote, data_toclient;
    char buf_toremote[DATA_SIZE];
    char buf_toclient[DATA_SIZE];
    connection_list *next, *prev;
};

connection_list *head, *tail;
int fd_max;

void set_fd_max(int fd){
    if(fd_max < fd+1)
        fd_max = fd + 1;
}

void add(int new_connection_fd, addrinfo *servinfo){
    connection_list *q = (connection_list*)malloc(sizeof(connection_list));
    q->client_socket = new_connection_fd;
    q->remote_socket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    set_fd_max(q->remote_socket);
    int res = connect(q->remote_socket, servinfo->ai_addr, sizeof(*servinfo->ai_addr));
    if(res != 0){
        if(errno != EINPROGRESS) {
            perror("connecting to server error:");
            close(q->remote_socket);
            exit(3);
        }
    }
    q->prev = NULL;
    if(head != NULL) {
        q->next = head;
        head->prev = q;
    }else{
        q->next = NULL;
        tail = q;
    }
    q->data_toclient = 0;
    q->data_toremote = 0;
    memset(q->buf_toremote, 0, sizeof(q->buf_toremote));
    memset(q->buf_toclient, 0, sizeof(q->buf_toclient));
    head = q;
}

void remove(connection_list *q){
    if(q == head && q == tail) {
        head = NULL;
        tail = NULL;
    } else if(q == head) {
        head = head->next;
        head->prev = NULL;
    } else if(q == tail) {
        tail = tail->prev;
        tail->next = NULL;
    } else {
        q->prev->next = q->next;
        q->next->prev = q->prev;
    }
    close(q->client_socket);
    close(q->remote_socket);
    free(q);
}