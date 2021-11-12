#include <stdio.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

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

// Timer
// the duration is in seconds
#define TIMER_DURATION 1

int startConnection(int sock, struct sockaddr_in* socketAdressPertu, struct sockaddr_in* socketAdressLocal, char* messAck, char* messSrc, socklen_t len) {
    // add timeout
    struct timeval timeout;
    timeout.tv_sec = TIMER_DURATION;
    timeout.tv_usec = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(struct timeval)) < 0) {
        perror("Problème au niveau du timeout \n");
    }

    // Prepare the data to send
    messAck[TYPE] = SYN + ACK;
    messAck[NUM_SEQ] = rand() % 256;
    messAck[NUM_ACK] = messSrc[NUM_SEQ] + 1;

    do {
        if (sendto(sock, messAck, BUFF, 0, (struct sockaddr*)socketAdressPertu, len) == -1) {
            perror("Problème au niveau du sendto");
            exit(0);
        }

        // get the ack from the source
        if (recvfrom(sock, messSrc, BUFF, 0, (struct sockaddr*)socketAdressLocal, &len) == -1) {
            if (errno == EWOULDBLOCK) {
                printf("Timeout \n");
            } else {
                perror("Erreur au niveau du recvfrom \n");
                exit(0);
            }
        }
    } while (messSrc[TYPE] != ACK && messSrc[NUM_SEQ] != messAck[NUM_ACK] + 1);

    // delete timeout
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(struct timeval)) < 0) {
        perror("Problème au niveau du timeout \n");
    }
    printf("Connexion établie cote serveur");
    return TRUE;
}

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
    const in_addr_t ip = inet_addr(argv[1]);
    struct sockaddr_in socketAdressPertu, socketAdressLocal;
    socklen_t len = sizeof(struct sockaddr_in);
    char messSrc[BUFF];
    char messAck[BUFF];
    int isConEnable = FALSE;
    int nbrTransmissionFailed = 0;
    // log file
    FILE * fileToWrite;
    fileToWrite = fopen("recvUDP.txt", "w");

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
    // ajout d'un timeout sur la socket initialisé à 0 (pas de timeout)
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(struct timeval)) < 0) {
        perror("Problème au niveau du timeout \n");
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

    // Démarage de la boucle principal
    int numSeq = 0;
    do {
        printf("\n----------------\n");
        // wait to get the message from the sender
        if (recvfrom(sock, messSrc, BUFF, 0, (struct sockaddr*)&socketAdressLocal, &len) == -1) {
            perror("Problème au niveau du résultat");
            exit(0);
        }
        // Copy the ID, ECN, C_WINDOW and DATA from the source to the serveur.
        messAck[ID_FLUX] = messSrc[ID_FLUX];
        messAck[ECN] = messSrc[ECN];
        messAck[C_WINDOW] = messSrc[C_WINDOW];
        messAck[DATA] = 0;

        // if the type of the message is 1 (SYN), send an acknowledgement with SYN+ACK
        if (messSrc[TYPE] == SYN) {
            isConEnable = startConnection(sock, &socketAdressPertu, &socketAdressLocal, messAck, messSrc, len);
        }
        exit(0);
        if (isConEnable == TRUE && messSrc[TYPE] == 0) {
            printf("numSeq attendu : %d \n", numSeq);
            if (messSrc[NUM_SEQ] != numSeq) {
                printf("Message reçu avec numéro de séquence incorrect \n");
                nbrTransmissionFailed++;
            } else {
                fputs(&messSrc[DATA], fileToWrite);
                numSeq = (numSeq + 1)%2;
            }
            // send the ACK to the client
            messAck[TYPE] = ACK;
            messAck[NUM_SEQ] = messSrc[NUM_SEQ];
            messAck[NUM_ACK] = messSrc[NUM_SEQ];
            if (sendto(sock, messAck, BUFF, 0, (struct sockaddr*)&socketAdressPertu, len) == -1) {
                perror("Problème au niveau du sendto");
                exit(0);
            } else {
                printf("Message envoyé avec bit à %d \n", messAck[NUM_ACK]);
            }
        }

        char ipSrc[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &socketAdressLocal.sin_addr, ipSrc, INET_ADDRSTRLEN);
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
    fclose(fileToWrite);
    printf("\n Incorrect number : %d", nbrTransmissionFailed);
    return 0;
}
