//
// Created by anthony on 18/09/2021.
//
#include <stdio.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 3567
#define BUFF 512

int main(int argc, char *argv[]) {

    char mess[BUFF];
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in adressSrc;
    struct sockaddr_in adressOther;
    adressSrc.sin_family = AF_INET;
    adressSrc.sin_addr.s_addr = INADDR_ANY;
    adressSrc.sin_port = htons(PORT);
    bzero(&(adressSrc.sin_zero), 8);
    socklen_t len = sizeof(struct sockaddr_in);

    if (sock == -1) {
        perror("Erreur au niveau du socket");
        exit(1);
    }

    int binded = bind(sock, (struct sockaddr *)&adressSrc, len);
    if (binded == -1) {
        perror("Erreur au niveau du bind");
        exit(1);
    }

    int listening = listen(sock, 10);
    if (listening == -1) {
        perror("Erreur au niveau du listen");
        exit(1);
    }

    int currentSocket = accept(sock, (struct sockaddr *)&adressOther, &len);
    if (currentSocket == -1) {
        perror("Erreur au niveau du connect");
    } else {
        printf("Addresse IP : %s \n", inet_ntoa(adressOther.sin_addr));
        printf("Port : %hu \n", ntohs(adressOther.sin_port));
    }

    ssize_t sizeGot = recv(currentSocket, mess, BUFF, 0);
    if (sizeGot == -1) {
        perror("Erreur au niveau de la reception");
    } else {
        printf("Message : %s", mess);
    }

    close(currentSocket);
    return 0;
}