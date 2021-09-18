//
// Created by anthony on 18/09/2021.
//
#include <stdio.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>

#define PORT 3567
#define BUFF 512

int main(int argc, char *argv[]) {
    char mess[BUFF] = "Hello l\'equipe, ca va ou quoi ?";
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

    int connected = connect(sock, (struct sockaddr *)&adressSrc, len);
    if (connected == -1) {
        perror("Erreur au niveau de la connection");
        exit(1);
    }

    ssize_t sended = send(sock, mess, BUFF, 0);
    if (sended == -1) {
        perror("Erreur au niveau de l'envoie");
    }

    printf("Listening with a size of %zd\n", sended);
    return 0;
}