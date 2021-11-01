#include <stdio.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#define BUFF 52

/*
 * Run with 3 args :
 * - string <IP_DEST>
 * - int <num_port>
 * - string <message>
 */
int main(int argc, char *argv[]) {

    const long port = strtol(argv[2], NULL, 10);
    const in_addr_t ipDest = inet_addr(argv[1]);
    struct sockaddr_in socketAdress;
    socklen_t len = sizeof socketAdress;
    char mess[52];
    // Id du Flux
    mess[0] = 25;
    // Type
    mess[1] = 0;
    // num séquence
    mess[2] = 5;
    // num Acquittement
    mess[4] = 6;
    // ECN
    mess[6] = 0;
    // Fen. Emission
    mess[7] = 52;

    sprintf(&mess[8], "%s", "salut les kidou comment ça va ? ");

    printf("len addr : %c \n", mess[8]);
    printf("len addr : %c \n", mess[9]);
    printf("len addr : %c \n", mess[10]);
    if (strlen(argv[3]) > 351) {
        printf("Votre message est trop grand ! Il doit faire moins 512 caractere.");
        exit(0);
    }

//    sprintf(mess, "%s", argv[3]);
//    sprintf(data, "%s", "Solido pugnantium iam ductores concitat alacriter anceps in subire quorum dolorem sed certamen hastisque terrebat quorum alacriter occurrere longe cum iram cunctorum et iram eum habitus consurgentem revocavere concitat iram habitus miles certamen dolorem in locari iam pugnantium qui qui certamen intempestivum quorum scuta iam iam quiquzhd iuqzh diuh");
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

    // Envoie d'un message vers la destination et vérification de l'envoie.
    ssize_t sended = sendto(sock, mess, 52, 0, (struct sockaddr*)&socketAdress, len);
    if (sended == -1) {
        perror("Problème au niveau de l'envoie");
    } else {
        printf("Envoie de %zd octet", sended);
    }

    return 0;
}