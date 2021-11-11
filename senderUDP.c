#include <stdio.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <time.h>

#define TRUE 1
#define FALSE 0

#define BUFF 52
#define ID_FLUX 0
#define TYPE 1
#define NUM_SEQ 2
#define NUM_ACK 4
#define ECN 6
#define C_WINDOW 7
#define DATA 8

// DataType
#define SYN 1
#define FIN 2
#define RST 4
#define ACK 16

// ECN type
#define ECN_ENABLE 1
#define ECN_DISABLE 0
/*
 * Run with 4 args :
 * - string <mode>
 * - string <IP_DEST>
 * - int <num_port_local>
 * - int <num_port_pert>
 */
int main(int argc, char *argv[]) {
    // generate the random number seed
    srand(time(NULL));
    const long port_local = strtol(argv[3], NULL, 10);
    const long port_pertu = strtol(argv[4], NULL, 10);
    const in_addr_t ipDest = inet_addr(argv[2]);
    struct sockaddr_in socketAdressLocal;
    struct sockaddr_in socketAdressPertu;
    socklen_t len = sizeof socketAdressLocal;
    char messPertu[BUFF];
    char messLocal[BUFF];
    int isConEnable = FALSE;

    // vérification que le port est été saisie.
    if(port_local == 0 ) {
        perror("Erreur au niveau du port de destination \n");
        exit(0);
    }
    // vérification que le port pertu est saisie
    if(port_pertu == 0 ) {
        perror("Erreur au niveau du port de perturbation \n");
        exit(0);
    }

    // Initialisation du socket et test d'erreur
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == -1) {
        perror("Problème au niveau du socket \n");
    }

    // définition d'une structure d'adresse avec l'ip de la machine pertubateur
    socketAdressPertu.sin_family = AF_INET;
    socketAdressPertu.sin_addr.s_addr = htonl(ipDest);;
    socketAdressPertu.sin_port = htons(port_pertu);
    bzero(&(socketAdressPertu.sin_zero), 8);

    // définition d'une structure d'adresse avec l'ip de la machine local
    socketAdressLocal.sin_family = AF_INET;
    socketAdressLocal.sin_addr.s_addr = htonl(INADDR_ANY);
    socketAdressLocal.sin_port = htons(port_local);
    bzero(&(socketAdressLocal.sin_zero), 8);

    // on lie le port avec le socket et on test l'erreur.
    int b = bind(sock, (struct sockaddr*)&socketAdressLocal, sizeof(struct sockaddr));
    if (b == -1) {
        perror("Problème au niveau du bind !");
        exit(0);
    }

    // Start 3 way handshake
    // Send SYN to start connection
    // Id du Flux
    messLocal[ID_FLUX] = 1;
    // Type
    messLocal[TYPE] = SYN;
    // num séquence
    messLocal[NUM_SEQ] = rand() % 256;
    // num Acquittement
    messLocal[NUM_ACK] = 0;
    // ECN
    messLocal[ECN] = ECN_DISABLE;
    // Fen. Emission
    messLocal[C_WINDOW] = 52;

    // send a the SYN message
    if (sendto(sock, messLocal, BUFF, 0, (struct sockaddr *) &socketAdressPertu, len) == -1) {
        perror("Erreur au niveau du sendto \n");
        exit(0);
    }

    // get the response message and check if the result is SYN + ACK
    if (recvfrom(sock, messPertu, BUFF, 0, (struct sockaddr *) &socketAdressLocal, &len) == -1) {
        perror("Erreur au niveau du recvfrom \n");
        exit(0);
    }
    if (messPertu[TYPE] == SYN + ACK) {
        printf("Connexion établie du côté de la source");
        isConEnable = TRUE;
        // send the ACK message
        messLocal[TYPE] = ACK;
        messLocal[NUM_SEQ] = messPertu[NUM_ACK] + 1;
        messLocal[NUM_ACK] = messPertu[NUM_SEQ] + 1;
        if (sendto(sock, messLocal, BUFF, 0, (struct sockaddr *) &socketAdressPertu, len) == -1) {
            perror("Erreur au niveau du sendto \n");
            exit(0);
        }
    }


    // Envoie d'une série de message
//    for (int i = 0; i < 100; ++i) {
//        // chargement du message
//        sprintf(&mess[DATA], "%s", "salut les kidou comment ça va ? ");
//        // Envoie d'un message vers la destination et vérification de l'envoie.
//        sended = sendto(sock, mess, 52, 0, (struct sockaddr*)&socketAdress, len);
//        if (sended == -1) {
//            perror("Problème au niveau de l'envoie");
//        } else {
//            printf("Envoie de %zd octet \n", sended);
//        }
//    }
    // Envoie d'un message vde fin de connection.
//    mess[TYPE] = 2;
//    sended = sendto(sock, mess, 52, 0, (struct sockaddr*)&socketAdress, len);
//    if (sended == -1) {
//        perror("Problème au niveau de l'envoie");
//    } else {
//        printf("Envoie de la fin \n");
//    }

    return 0;
}