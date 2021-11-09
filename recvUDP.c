#include <stdio.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>

#define BUFF 52
#define BUFF 52
#define ID_FLUX 0
#define TYPE 1
#define NUM_SEQ 2
#define NUM_ACK 4
#define ECN 6
#define C_WINDOW 7
#define DATA 8

/*
 * Run with 1 args : int numport
 */
int main(int argc, char *argv[]) {

    const long port = strtol(argv[1], NULL, 10);
    socklen_t len = sizeof(struct sockaddr_in);
    char messSrc[BUFF];
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

    do {
        // wait to get the messSrcage from the sender
        ssize_t result = recvfrom(sock, messSrc, BUFF, 0, (struct sockaddr*)&socketAdress, &len);
        if (result == -1) {
            perror("Problème au niveau du résultat");
            exit(0);
        } else {
            char ipSrc[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &socketAdress.sin_addr, ipSrc, INET_ADDRSTRLEN);
            printf("\n----------------\n");
            printf("ID du flux : %d \n", messSrc[ID_FLUX]);
            printf("Type du flux : %d\n", messSrc[TYPE]);
            printf("NUméro de seq %d \n", messSrc[NUM_SEQ]);
            printf("Numéro d'ack %d \n", messSrc[NUM_ACK]);
            printf("ECN Enable ? %d \n", messSrc[ECN]);
            printf("Fenetre Emission %d \n", messSrc[C_WINDOW]);
            printf("message : %s \n", &messSrc[DATA]);
            printf("Octet recu : %zd \n", result);
            printf("Caractère recu : %s \n", ipSrc);
            printf("Port recu : %hu \n", ntohs(socketAdress.sin_port));
            printf("\n");
        }
    } while (messSrc[TYPE] != 2);
    return 0;
}
