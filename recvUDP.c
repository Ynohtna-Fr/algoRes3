#include <stdio.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <string.h>

#define TRUE 1
#define FALSE 0

#define MESSAGE_BUFF 52
#define MAX_SEQ 65534

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

typedef struct packet {
    unsigned char flux_id;
    unsigned char type;
    unsigned short seq;
    unsigned short ack;
    unsigned char ecn;
    unsigned char e_window;
    char data[44];
} *packet;

int startConnection(int sock, struct sockaddr_in* socketAdressPertu, struct sockaddr_in* socketAdressLocal, packet packetAck, packet packetSrc, socklen_t len) {
    char messSrc[MESSAGE_BUFF];
    char messAck[MESSAGE_BUFF];

    // add timeout
    struct timeval timeout;
    timeout.tv_sec = TIMER_DURATION;
    timeout.tv_usec = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(struct timeval)) < 0) {
        perror("Problème au niveau du timeout \n");
    }

    // Prepare the data to send
    packetAck->type = SYN + ACK;
    packetAck->seq = rand() % MAX_SEQ;
    packetAck->ack = packetSrc->seq + 1;
    memcpy(messAck, packetAck, sizeof(struct packet));
    do {
        if (sendto(sock, messAck, MESSAGE_BUFF, 0, (struct sockaddr*)socketAdressPertu, len) == -1) {
            perror("Problème au niveau du sendto");
            exit(0);
        }

        // get the ack from the source
        if (recvfrom(sock, messSrc, MESSAGE_BUFF, 0, (struct sockaddr*)socketAdressLocal, &len) == -1) {
            if (errno == EWOULDBLOCK) {
                printf("Timeout \n");
            } else {
                perror("Erreur au niveau du recvfrom \n");
                exit(0);
            }
        } else {
            memcpy(packetSrc, messSrc, sizeof(struct packet));
        }
    } while (packetSrc->type != ACK && packetSrc->seq != packetAck->type + 1);

    // delete timeout
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(struct timeval)) < 0) {
        perror("Problème au niveau du timeout \n");
    }
    printf("Connexion établie cote serveur");
    return TRUE;
}

int endConnection(int sock, struct sockaddr_in *socketAdressLocal, struct sockaddr_in *socketAdressPertu, packet packetPertu) {
    packet packetLocal = malloc(sizeof(struct packet));
    char messPertu[MESSAGE_BUFF];
    char messLocal[MESSAGE_BUFF];
    socklen_t len = sizeof(struct sockaddr_in);

    printf("seq : %d, type : %d, ack : %d \n", packetPertu->seq, packetPertu->type, packetPertu->ack);
    // packet initialization
    packetLocal->flux_id = 1;
    packetLocal->type = ACK;
    packetLocal->seq = packetPertu->seq;
    packetLocal->ack = packetPertu->seq + 1;
    packetLocal->ecn = ECN_DISABLE;
    packetLocal->e_window = 1;
    sprintf(packetLocal->data, "%s", "FIN");
    memcpy(messLocal, packetLocal, sizeof(struct packet));
    // send the packet
    if (sendto(sock, messLocal, MESSAGE_BUFF, 0, (struct sockaddr*)socketAdressPertu, len) == -1) {
        perror("Problème au niveau du sendto");
        exit(0);
    }

    packetLocal->type = FIN;
    packetLocal->seq = rand() % MAX_SEQ;
    memcpy(messLocal, packetLocal, sizeof(struct packet));
    do {
        if (sendto(sock, messLocal, MESSAGE_BUFF, 0, (struct sockaddr*)socketAdressPertu, len) == -1) {
            perror("Problème au niveau du sendto");
            exit(0);
        }
        // recv the ack from the source
        if (recvfrom(sock, messPertu, MESSAGE_BUFF, 0, (struct sockaddr*)socketAdressLocal, &len) == -1) {
            if (errno == EWOULDBLOCK) {
                printf("Timeout \n");
            } else {
                perror("Erreur au niveau du recvfrom \n");
                exit(0);
            }
        } else {
            memcpy(packetPertu, messPertu, sizeof(struct packet));
        }
    } while (packetPertu->type != ACK && packetPertu->ack != packetLocal->seq + 1);

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
    socklen_t len = sizeof(struct sockaddr_in);
    packet packetSrc = malloc(sizeof(struct packet));
    packet packetAck = malloc(sizeof(struct packet));
    int isConEnable = FALSE;
    int end = FALSE;
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

    // définition d'une structure d'adresse avec l'ip de la machine local et perturbateur
    struct sockaddr_in socketAdressPertu, socketAdressLocal;
    socketAdressLocal.sin_family = AF_INET;
    socketAdressLocal.sin_addr.s_addr = htonl(ip);;
    socketAdressLocal.sin_port = htons(port_local);
    bzero(&(socketAdressLocal.sin_zero), 8);

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
    int numSeqSW = 0; // seq number wanted if we'r in stop and wait
    int numSeqGBN = 0; // seq number wanted if we'r in go back n
    char messSrc[MESSAGE_BUFF];
    char messAck[MESSAGE_BUFF];
    char messToPrint[MESSAGE_BUFF];
    do {
        time_t timestamp = time( NULL );
        packetSrc->seq = 0;
        printf("\n----------------\n");
        // wait to get the message from the sender
        if (recvfrom(sock, messSrc, MESSAGE_BUFF, 0, (struct sockaddr*)&socketAdressLocal, &len) == -1) {
            perror("Problème au niveau du résultat");
            exit(0);
        }
        memcpy(packetSrc, messSrc, sizeof(struct packet));
        // Copy the ID, ECN, C_WINDOW and DATA from the source to the serveur.
        packetAck->flux_id = packetSrc->flux_id;
        packetAck->ecn = packetSrc->ecn;
        packetAck->e_window = packetSrc->e_window;
        packetAck->ack = 0;
        // if the type of the message is 1 (SYN), send an acknowledgement with SYN+ACK
        if (packetSrc->type == SYN) {
            isConEnable = startConnection(sock, &socketAdressPertu, &socketAdressLocal, packetAck, packetSrc, len);
        }

        if (isConEnable == TRUE && packetSrc->type == 0) {
            if (packetSrc->seq != numSeqSW && packetSrc->seq != numSeqGBN) {
                printf("Message reçu avec numéro de séquence incorrect \n");
                nbrTransmissionFailed++;
                sprintf(messToPrint, "%ld, %d, false\n", timestamp, packetSrc->seq);
            } else {
                sprintf(messToPrint, "%ld, %d, true\n", timestamp, packetSrc->seq);
                numSeqSW = (numSeqSW + 1)%2;
                numSeqGBN = (numSeqGBN + 1)%MAX_SEQ;
            }
            fputs(messToPrint, fileToWrite);
            // send the ACK to the client
            packetAck->type = ACK;
            packetAck->ack = packetSrc->seq;
            if (packetSrc->seq > 1) { // ça veut dire qu'on est en go back n
                packetAck->ack = numSeqGBN -1 ;
            }
            memcpy(messAck, packetAck, sizeof(struct packet));
            if (sendto(sock, messAck, MESSAGE_BUFF, 0, (struct sockaddr*)&socketAdressPertu, len) == -1) {
                perror("Problème au niveau du sendto");
                exit(0);
            } else {
                printf("Message envoyé avec bit à %d \n", packetAck->ack);
            }
        }

        if (isConEnable == TRUE && packetSrc->type == FIN) {
            end = endConnection(sock, &socketAdressLocal, &socketAdressPertu, packetSrc);
        }

        printf("ID du flux : %d \n", packetSrc->flux_id);
        printf("Type du flux : %d\n", packetSrc->type);
        printf("NUméro de seq %d \n", packetSrc->seq);
        printf("Numéro d'ack %d \n", packetSrc->ack);
        printf("ECN Enable ? %d \n", packetSrc->ecn);
        printf("Fenetre Emission %d \n", packetSrc->e_window);
        printf("message : %s \n", packetSrc->data);
        printf("\n");
    } while (end == FALSE);

    printf("\n Incorrect number : %d", nbrTransmissionFailed);
    fclose(fileToWrite);
    return 0;
}
