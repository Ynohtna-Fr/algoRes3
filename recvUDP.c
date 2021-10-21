#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>

#define BUFF 512

/*
 * Run with 1 args : int numport
 */
int main(int argc, char *argv[]) {
    const long port = strtol(argv[1], NULL, 10);
    socklen_t len = sizeof(struct sockaddr_in);
    char mess[BUFF];
    struct sockaddr_in socketAdress;

    // vérification que le port est été saisie.
    if(port == 0 ) {
        perror("Erreur au niveau du port de destination");
        exit(0);
    }

    // Initialisation du socket et test d'erreur
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (sock == -1) {
        perror("Problème au niveau du socket");
        exit(0);
    }

    // définition d'une structure d'adresse avec l'ip de la machine
    socketAdress.sin_family = AF_INET;
    socketAdress.sin_addr.s_addr = htonl(INADDR_ANY);;
    socketAdress.sin_port = htons(port);
    bzero(&(socketAdress.sin_zero), 8);

    // on lie le port avec le socket et on test l'erreur.
    int b = bind(sock, (struct sockaddr*)&socketAdress, sizeof(struct sockaddr));
    if (b == -1) {
        perror("Problème au niveau du bind !");
        exit(0);
    }

    // wait to get the message from the sender
    ssize_t result = recvfrom(sock, mess, BUFF, 0, (struct sockaddr*)&socketAdress, &len);

    if (result == -1) {
        perror("Problème au niveau du résultat");
        exit(0);
    } else {
        char ipSrc[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &socketAdress.sin_addr, ipSrc, INET_ADDRSTRLEN);
        printf("Message : %s \n", mess);
        printf("Paquet recu : %zd \n", result);
        printf("Caractère recu : %lu \n", strlen(mess));
        printf("Caractère recu : %s \n", ipSrc);
        printf("Port recu : %ld \n", port);
    }
    return 0;
}