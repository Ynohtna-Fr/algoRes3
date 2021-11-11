#include <stdio.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
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
 * Run with 3 args :
 * - string <ip_dest>
 * - int <port_local>
 * - int <port_pertu>
 */
int main(int argc, char *argv[]) {
    // generate the random number seed
    srand(time(NULL));
    const long port_local = strtol(argv[2], NULL, 10);
    const long port_pertu = strtol(argv[3], NULL, 10);
    socklen_t len = sizeof(struct sockaddr_in);
    const in_addr_t ip = inet_addr(argv[1]);
    struct sockaddr_in socketAdressPertu;
    struct sockaddr_in socketAdressLocal;
    char messSrc[BUFF];
    char messAck[BUFF];
    int isConEnable = FALSE;

    // vérification que le port est été saisie.
    if(port_local == 0 ) {
        perror("Erreur au niveau du port de destination");
        exit(0);
    }
    // vérification que le port pertu à été saisie
    if(port_pertu == 0 ) {
        perror("Erreur au niveau du port de perturbation");
        exit(0);
    }

    // Initialisation du socket et test d'erreur
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == -1) {
        perror("Problème au niveau du socket");
        exit(0);
    }

    // définition d'une structure d'adresse avec l'ip de la machine
    socketAdressLocal.sin_family = AF_INET;
    socketAdressLocal.sin_addr.s_addr = htonl(ip);;
    socketAdressLocal.sin_port = htons(port_local);
    bzero(&(socketAdressLocal.sin_zero), 8);

    // définition d'une structure d'adresse avec l'ip de la machine pertubateur
    socketAdressPertu.sin_family = AF_INET;
    socketAdressPertu.sin_addr.s_addr = htonl(INADDR_ANY);;
    socketAdressPertu.sin_port = htons(port_pertu);
    bzero(&(socketAdressPertu.sin_zero), 8);

    // on lie le port avec le socket et on test l'erreur.
    int b = bind(sock, (struct sockaddr*)&socketAdressLocal, sizeof(struct sockaddr));
    if (b == -1) {
        perror("Problème au niveau du bind !");
        exit(0);
    }

    do {
        // wait to get the messSrcage from the sender
        if (recvfrom(sock, messSrc, BUFF, 0, (struct sockaddr*)&socketAdressLocal, &len) == -1) {
            perror("Problème au niveau du résultat");
            exit(0);
        }

        // if the type of the message is 1 (SYN), send an acknowledgement with SYN+ACK
        if (messSrc[TYPE] == SYN) {
            messAck[ID_FLUX] = messSrc[ID_FLUX];
            messAck[TYPE] = SYN + ACK;
            messAck[NUM_SEQ] = rand() % 256;
            messAck[NUM_ACK] = messSrc[NUM_SEQ] + 1;
            messAck[ECN] = 0;
            messAck[C_WINDOW] = messSrc[C_WINDOW];
            messAck[DATA] = ECN_DISABLE;
            // check if the message was send correctly
            if (sendto(sock, messAck, BUFF, 0, (struct sockaddr*)&socketAdressPertu, len) == -1) {
                perror("Problème au niveau du sendto");
                exit(0);
            }

            // get the ack from the source
            if (recvfrom(sock, messSrc, BUFF, 0, (struct sockaddr*)&socketAdressLocal, &len) == -1) {
                perror("Problème au niveau du résultat");
                exit(0);
            }
            if (messSrc[TYPE] == ACK) {
                printf("Connexion établie cote serveur");
                isConEnable = TRUE;
            }
        }

        char ipSrc[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &socketAdressLocal.sin_addr, ipSrc, INET_ADDRSTRLEN);
        printf("\n----------------\n");
        printf("ID du flux : %d \n", messSrc[ID_FLUX]);
        printf("Type du flux : %d\n", messSrc[TYPE]);
        printf("NUméro de seq %d \n", messSrc[NUM_SEQ]);
        printf("Numéro d'ack %d \n", messSrc[NUM_ACK]);
        printf("ECN Enable ? %d \n", messSrc[ECN]);
        printf("Fenetre Emission %d \n", messSrc[C_WINDOW]);
        printf("message : %s \n", &messSrc[DATA]);
        printf("Caractère recu : %s \n", ipSrc);
        printf("Port recu : %hu \n", ntohs(socketAdressLocal.sin_port));
        printf("\n");
    } while (messSrc[TYPE] != FIN);
    return 0;
}
