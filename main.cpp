#include <stdio.h>
#include <cstdlib>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <zconf.h>
#include "connection_list.h"


void set_descriptors(fd_set *readfs, fd_set *writefs, int listenfd){
    connection_list *q = head;
    FD_ZERO(readfs);
    FD_ZERO(writefs);
    FD_SET(listenfd, readfs);

    while(q != NULL){
        if((q->data_toclient < 0 && q->data_toremote<=0) || q->data_toclient <= 0 && q->data_toremote<0 ){
            remove(q);
        } else {
            if(q->data_toremote ==0)
                FD_SET(q->client_socket, readfs);
            if(q->data_toclient==0)
                FD_SET(q->remote_socket, readfs);
            if(q->data_toremote>0)
                FD_SET(q->remote_socket, writefs);
            if(q->data_toclient>0)
                FD_SET(q->client_socket, writefs);
        }
        q = q->next;
    }

}

void move_bufdata(char *buf, int last_datasize, int writed){
    memmove(buf, buf + writed, last_datasize - writed);
}

void handle_descriptors(fd_set *readfs, fd_set *writefs){
    connection_list *q = head;
    int writed;
    while (q != NULL){
        if(q->data_toremote == 0 && FD_ISSET(q->client_socket,readfs)){
            q->data_toremote = read(q->client_socket,q->buf_toremote,sizeof(q->buf_toremote));
            if(q->data_toremote == 0)
                q->data_toremote = -1;
        }
        if(q->data_toclient == 0 && FD_ISSET(q->remote_socket,readfs)){
            q->data_toclient = read(q->remote_socket,q->buf_toclient,sizeof(q->buf_toclient));
            if(q->data_toclient == 0)
                q->data_toclient = -1;
        }
        if(q->data_toremote > 0 && FD_ISSET(q->remote_socket,writefs)){
            writed = write(q->remote_socket,q->buf_toremote,q->data_toremote);
            if(writed == -1)
                q->data_toclient = -1;
            else if(writed == q->data_toremote)
                q->data_toremote = 0; ///нет учета того, что записаться может не все.
            else{
                move_bufdata(q->buf_toremote,q->data_toremote,writed);
                q->data_toremote -= writed;
            }
        }
        if(q->data_toclient > 0 && FD_ISSET(q->client_socket,writefs)){
            writed = write(q->client_socket,q->buf_toclient,q->data_toclient);
            if(writed == -1)
                q->data_toremote = -1;
            else if(writed == q->data_toclient)
                q->data_toclient = 0; ///нет учета того, что записаться может не все.
            else{
                move_bufdata(q->buf_toclient,q->data_toclient,writed);
                q->data_toclient -= writed;
            }
        }

        q = q->next;
    }
}

int main(int argc, char** argv){
    if(argc != 4){
        printf("Wrong parameters");
        return 0;
    }
    int listenfd;
    int lport, rport;
    int status;
    int new_connection_fd;
    struct addrinfo hints;
    struct addrinfo *servinfo;
    struct sockaddr_in rhost_addr, lhost_addr, newclient_addr;
    fd_set	readfs, writefs;
    fd_max = 0;

    lport=atoi(argv[1]); //port to listen
    //rport=atoi(argv[3]);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;     // ipv4
    hints.ai_socktype = SOCK_STREAM; // TCP stream-sockets
    hints.ai_flags = AI_PASSIVE;     // put my ip automatically

    if ((status = getaddrinfo(argv[2], argv[3], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo error: %sn", gai_strerror(status));
        exit(1);
    }


    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    set_fd_max(listenfd);

    lhost_addr.sin_family      = AF_INET;
    lhost_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    lhost_addr.sin_port        = htons(lport);

    if (bind(listenfd, (struct sockaddr *) &lhost_addr, sizeof(lhost_addr))) {  //связывание сокета с адресом и номером порта
        perror("bind error:");
        return 0;
    }

    listen(listenfd,SOMAXCONN);
    int ready_fd;

    while (true){
        set_descriptors(&readfs, &writefs, listenfd);
        ready_fd = select(fd_max, &readfs, &writefs, NULL, NULL);
        if(ready_fd == 0){
            continue;
        } else if(ready_fd < 0){
            perror("select errror:");
            exit(1);
        }
        //printf("current number of fd: %d\n",ready_fd);
        handle_descriptors(&readfs,&writefs);
        if(FD_ISSET(listenfd,&readfs)){
            socklen_t addrlen = sizeof(newclient_addr);
            new_connection_fd = accept(listenfd, (struct sockaddr *)&newclient_addr, &addrlen);
            //printf("new connection from port: %d\n",newclient_addr.sin_port);
            if(new_connection_fd < 0){
                perror("accept error:");
                exit(2);
            }
            set_fd_max(new_connection_fd);
            add(new_connection_fd, servinfo);
        }
    }




    freeaddrinfo(servinfo);

}