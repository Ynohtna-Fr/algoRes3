#include <stdio.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#define BUFF 512

/*
 * Run with 3 args :
 * - string <IP_DEST>
 * - int <num_port>
 * - string <file_path>
 */
int main(int argc, char *argv[]) {
    const long port = strtol(argv[2], NULL, 10);
    const in_addr_t ipDest = inet_addr(argv[1]);
    struct sockaddr_in socketAdress;
    socklen_t len = sizeof socketAdress;
    char mess[BUFF];
    FILE * fileToSend;

    if (argc < 3) {
        printf("Il vous manque des arguments");
        exit(0);
    }

    fileToSend = fopen(argv[3], "r");

    if (fileToSend == NULL) {
        perror("Erreur avec le fichier");
        exit(1);
    }

    // vérification que le port est été saisie.
    if(port == 0 ) {
        perror("Erreur au niveau du port de destination \n");
        exit(0);
    }

    // Initialisation du socket et test d'erreur
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == -1) {
        perror("Problème au niveau du socket \n");
    }

    // définition d'une structure d'adresse avec l'ip de la machine
    socketAdress.sin_family = AF_INET;
    socketAdress.sin_addr.s_addr = htonl(ipDest);;
    socketAdress.sin_port = htons(port);

    while (! feof(fileToSend)) {
        fgets(mess, BUFF, fileToSend);
        if (ferror(fileToSend)) {
            printf("Erreur");
        } else {
            // Envoie d'un message vers la destination et vérification de l'envoie.
            ssize_t sended = sendto(sock, mess, BUFF, 0, (struct sockaddr*)&socketAdress, len);
            if (sended == -1) {
                perror("Problème au niveau de l'envoie");
            } else {
                printf("Envoie de %zd octet \n", sended);
            }
        }
    }
    sprintf(mess, "%s", "END");
    ssize_t sended = sendto(sock, mess, BUFF, 0, (struct sockaddr*)&socketAdress, len);
    if (sended == -1) {
        perror("Problème au niveau de l'envoie");
    } else {
        printf("Envoie de %zd octet \n", sended);
    }
    fclose(fileToSend);
    return 0;
}
