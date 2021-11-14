#include <stdio.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <time.h>
#include <asm-generic/errno.h>
#include <errno.h>

#define TRUE 1
#define FALSE 0

#define MESSAGE_BUFF 52
#define MAX_SEQ 65534

#define ID_FLUX 0
#define TYPE 1
#define NUM_SEQ 2
#define NUM_ACK 4
#define ECN 6
#define E_WINDOW 7
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

// mode type
#define STOP_WAIT "stop-wait"
#define GO_BACK_N "go-back-n"

typedef struct packet {
    unsigned char flux_id;
    unsigned char type;
    unsigned short seq;
    unsigned short ack;
    unsigned char ecn;
    unsigned char e_window;
    char data[44];
} *packet;

void endConnection(int sock, struct sockaddr_in *socketAdressLocal, struct sockaddr_in *socketAdressPertu);

// start the connection with a 3-way handshake
int startConnection (const int sock, struct sockaddr_in* socketAdressLocal, struct sockaddr_in* socketAdressPertu) {
    packet packetLocal = malloc(sizeof(struct packet));
    packet packetPertu = malloc(sizeof(struct packet));
    char messPertu[MESSAGE_BUFF];
    char messLocal[MESSAGE_BUFF];
    socklen_t len = sizeof(struct sockaddr_in);
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
    messLocal[E_WINDOW] = 1;
    // Reset the type
    messPertu[TYPE] = 0;

    packetLocal->flux_id = 1;
    packetLocal->type = SYN;
    packetLocal->seq = rand() % MAX_SEQ;
    packetLocal->ack = 0;
    packetLocal->ecn = ECN_DISABLE;
    packetLocal->e_window = MESSAGE_BUFF;
    memcpy(messLocal, packetLocal, sizeof(struct packet));
    do {
        // send a the SYN message
        if (sendto(sock, messLocal, MESSAGE_BUFF, 0, (struct sockaddr *) socketAdressPertu, len) == -1) {
            perror("Erreur au niveau du sendtod \n");
            exit(0);
        }

        // get the response message and check if the result is SYN + ACK
        if (recvfrom(sock, messPertu, MESSAGE_BUFF, 0, (struct sockaddr *) socketAdressLocal, &len) == -1) {
            if (errno == EWOULDBLOCK) {
                printf("Timeout \n");
            } else {
                perror("Erreur au niveau du recvfrom \n");
                exit(0);
            }
        }
        memcpy(packetPertu, messPertu, sizeof(struct packet));
    } while ((packetPertu->type != SYN + ACK) && (packetPertu->ack != packetPertu->seq + 1));

    printf("Connexion établie du côté de la source \n");

    // send the ACK message
    packetLocal->type  = ACK;
    packetLocal->seq = packetPertu->ack + 1;
    packetLocal->ack = packetPertu->seq + 1;
    memcpy(messLocal, packetLocal, sizeof(struct packet));
    if (sendto(sock, messLocal, MESSAGE_BUFF, 0, (struct sockaddr *) socketAdressPertu, len) == -1) {
        perror("Erreur au niveau du sendto \n");
        exit(0);
    }

    return TRUE;
}

void stopAndWait(const int sock, struct sockaddr_in* socketAdressLocal, struct sockaddr_in* socketAdressPertu) {
    packet packetLocal = malloc(sizeof(struct packet));
    packet packetPertu = malloc(sizeof(struct packet));
    char messPertu[MESSAGE_BUFF];
    char messLocal[MESSAGE_BUFF];
    socklen_t len = sizeof(struct sockaddr_in);

    // packet initialization
    packetLocal->flux_id = 1;
    packetLocal->type = 0;
    packetLocal->seq = 0;
    packetLocal->ack = 0;
    packetLocal->ecn = ECN_DISABLE;
    packetLocal->e_window = 1;

    printf("\n Start stop and wait transmission \n");

    // Envoie d'une série de message
    for (int i = 0; i < 300; ++i) {
        printf("\n-----SEND-----\n");
        // reinitialisation du type de paquet reçu
        packetPertu->type = 0;

        // Message à envoyé
        sprintf(packetLocal->data, "%s %d %s", "salut ", i, " fin \n");

        printf("%s \n", packetLocal->data);

        do {
            memcpy(messLocal, packetLocal, sizeof(struct packet));
            // send the message
            if (sendto(sock, messLocal, MESSAGE_BUFF, 0, (struct sockaddr *) socketAdressPertu, len) == -1) {
                perror("Erreur au niveau du sendto \n");
                exit(0);
            }

            // Check if the response has timeout out
            if (recvfrom(sock, messPertu, MESSAGE_BUFF, 0, (struct sockaddr *) socketAdressLocal, &len) == -1) {
                // if the error correspond to EWOULDBLOCK print that
                if (errno == EWOULDBLOCK) {
                    printf("Timeout \n");
                } else {
                    perror("Erreur au niveau du recvfrom \n");
                    exit(0);
                }
            } else {
                memcpy(packetPertu, messPertu, sizeof(struct packet));
                // if the response is not a timeout, we're done but we need to check if the ack is correct
                if (packetPertu->type == ACK && packetPertu->ack == packetLocal->seq) {
                    printf(" ACK reçu \n");
                    packetLocal->seq = (packetLocal->seq + 1) % 2;
                } else {
                    printf("Problème d'ack\n");
                }
            }
        } while (packetPertu->type != ACK);
    }
}

void goBackN(int sock, struct sockaddr_in *socketAdressLocal, struct sockaddr_in *socketAdressPertu) {
    packet packetLocal = malloc(sizeof(struct packet));
    packet packetPertu = malloc(sizeof(struct packet));
    char messPertu[MESSAGE_BUFF];
    char messLocal[MESSAGE_BUFF];
    socklen_t len = sizeof(struct sockaddr_in);
    packet slidingWindows[10];
    int c_window = 3;
    int current_open_slot = c_window;
    int packetNum = 0;
    int last_ack = 0;
    struct timeval timeout = {0, 10};
    int end = FALSE;

    // modification du timeout sur la socket afin de simuler du non bloquant
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(struct timeval)) < 0) {
        perror("Problème au niveau du timeout \n");
    }

    // packet initialization
    packetLocal->flux_id = 1;
    packetLocal->type = 0;
    packetLocal->seq = 0;
    packetLocal->ack = 0;
    packetLocal->ecn = ECN_DISABLE;
    packetLocal->e_window = 1;

    printf("\n Start Go Back N transmission \n");

    // Envoie d'une série de message
    do {
        for (int i = 0; i < current_open_slot; ++i) {
            // send message
            printf("-------SEND----- \n");
            sprintf(packetLocal->data, "%s %d %s", "salut ", packetLocal->seq, " GBN \n");
            printf("%s \n", packetLocal->data);
            memcpy(messLocal, packetLocal, sizeof(struct packet));
            slidingWindows[packetNum] = packetLocal;
            if (sendto(sock, messLocal, MESSAGE_BUFF, 0, (struct sockaddr *) socketAdressPertu, len) == -1) {
                perror("Erreur au niveau du sendto \n");
                exit(0);
            }
            packetLocal->seq = packetLocal->seq + 1;
            packetNum = (packetNum + 1) % 10;
            current_open_slot--;
            printf("Congestion window : %d | current_open_slot : %d\n", c_window, current_open_slot);
        }

        if (recvfrom(sock, messPertu, MESSAGE_BUFF, 0, (struct sockaddr *) socketAdressLocal, &len) == -1) {
            if (errno == EWOULDBLOCK) {
                printf("-------Timeout------ \n");
                printf("Congestion window : %d | current_open_slot : %d\n", c_window, current_open_slot);
            } else {
                perror("Erreur au niveau du recvfrom \n");
                exit(0);
            }
        } else {
            memcpy(packetPertu, messPertu, sizeof(struct packet));
            printf("-------GET ACK------- \n");
            printf("%d \n", packetPertu->ack);
            if (packetPertu->ack != (last_ack + 1)) {
                int missingAck = 0;
                if (packetPertu->ack > last_ack) {
                    missingAck = (packetPertu->ack % 11) - (last_ack % 11);
                } else if (packetPertu->ack < last_ack) {
                    missingAck = ((packetPertu->ack % 11) + 11) + (last_ack % 11);
                }
                if (missingAck == 0) {
                    if (c_window < 10) {
                        c_window++;
                        current_open_slot++;
                    }
                    current_open_slot++;
                }
                for (int i = 0; i < missingAck; ++i) {
                    if (c_window < 10) {
                        c_window++;
                        current_open_slot++;
                    }
                    current_open_slot++;
                }
            } else {
                if (c_window < 10) {
                    c_window++;
                    current_open_slot++;
                }
                current_open_slot++;
            }
            if (packetPertu->ack >= 100) {
                end = TRUE;
            }
            last_ack = packetPertu->ack;
            printf("Congestion window : %d | current_open_slot : %d\n", c_window, current_open_slot);
        }
    } while (end == FALSE);

    // Remise de la socket en l'état les
    timeout.tv_sec = TIMER_DURATION;
    timeout.tv_usec = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(struct timeval)) < 0) {
        perror("Problème au niveau du timeout \n");
    }
}

void endConnection(int sock, struct sockaddr_in *socketAdressLocal, struct sockaddr_in *socketAdressPertu) {
    packet packetLocal = malloc(sizeof(struct packet));
    packet packetPertu = malloc(sizeof(struct packet));
    char messPertu[MESSAGE_BUFF];
    char messLocal[MESSAGE_BUFF];
    socklen_t len = sizeof(struct sockaddr_in);

    // packet initialization
    packetLocal->flux_id = 1;
    packetLocal->type = FIN;
    packetLocal->seq = rand() % MAX_SEQ;
    packetLocal->ack = 0;
    packetLocal->ecn = ECN_DISABLE;
    packetLocal->e_window = 1;
    sprintf(packetLocal->data, "%s", "FIN");
    memcpy(messLocal, packetLocal, sizeof(struct packet));

    if (sendto(sock, messLocal, MESSAGE_BUFF, 0, (struct sockaddr*) socketAdressPertu, len) == -1) {
        perror("Problème au niveau de l'envoie");
    } else {
        printf("Envoie de la fin \n");
    }
}
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
    char mode[10];
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
    // vérification que le mode est saisie est correspond soit à stop-and-wait soit à go-back-n
    if(strcmp(argv[1], STOP_WAIT) != 0 && strcmp(argv[1], GO_BACK_N) != 0) {
        perror("Erreur au niveau du mode \n");
        exit(0);
    }
    sprintf(mode, "%s", argv[1]);

    // Initialisation du socket et test d'erreur
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 500*1000; // 0.5 secondes
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == -1) {
        perror("Problème au niveau du socket \n");
    }
    // ajout d'un timeout sur la socket
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(struct timeval)) < 0) {
        perror("Problème au niveau du timeout \n");
    }

    // définition d'une structure d'adresse avec l'ip de la machine pertubateur et de la machine local
    struct sockaddr_in socketAdressLocal, socketAdressPertu;
    socketAdressPertu.sin_family = AF_INET;
    socketAdressPertu.sin_addr.s_addr = htonl(ipDest);;
    socketAdressPertu.sin_port = htons(port_pertu);
    bzero(&(socketAdressPertu.sin_zero), 8);

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
    isConEnable = startConnection(sock, &socketAdressLocal, &socketAdressPertu);

    // Create a Stop-and-wait transmission
    if (strcmp(mode, STOP_WAIT) == 0 && isConEnable == TRUE) {
        // Create a Stop-and-wait transmission
        stopAndWait(sock, &socketAdressLocal, &socketAdressPertu);
    } else if (strcmp(mode, GO_BACK_N) == 0 && isConEnable == TRUE) {
        // Create a Go-Back-N transmission
        goBackN(sock, &socketAdressLocal, &socketAdressPertu);
    }

    // close the connection with a 3way handshake
    endConnection(sock, &socketAdressLocal, &socketAdressPertu);

    return 0;
}


