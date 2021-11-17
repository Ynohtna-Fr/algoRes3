#include <stdio.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <time.h>
#include <asm-generic/errno.h>
#include <errno.h>
#include <math.h>

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

// mode type
#define STOP_WAIT "stop-wait"
#define GO_BACK_N "go-back-n"

// go back n
#define WINDOW_SIZE 10

typedef struct packet {
    unsigned char flux_id;
    unsigned char type;
    unsigned short seq;
    unsigned short ack;
    unsigned char ecn;
    unsigned char e_window;
    char data[44];
} *packet;

void flushBuffer(int sock,int c_window, struct sockaddr_in* socketAdressLocal) {
    char messPertu[MESSAGE_BUFF];
    socklen_t len = sizeof(struct sockaddr_in);
    for (int i = 0; i < MESSAGE_BUFF; ++i) {
        // on lis dans le vide pour vidé le buffer de la socket
        if (recvfrom(sock, messPertu, MESSAGE_BUFF, 0, (struct sockaddr *) socketAdressLocal, &len) == -1) {
            if (errno != EWOULDBLOCK) {
                perror("Erreur au niveau du recvfrom \n");
                exit(0);
            }
        }
    }
}

// start the connection with a 3-way handshake
int startConnection (const int sock, struct sockaddr_in* socketAdressLocal, struct sockaddr_in* socketAdressPertu) {
    packet packetLocal = malloc(sizeof(struct packet));
    packet packetPertu = malloc(sizeof(struct packet));
    char messPertu[MESSAGE_BUFF];
    char messLocal[MESSAGE_BUFF];
    socklen_t len = sizeof(struct sockaddr_in);

    packetLocal->flux_id = 1;
    packetLocal->type = SYN;
    packetLocal->seq = rand() % MAX_SEQ;
    packetLocal->ack = 0;
    packetLocal->ecn = ECN_DISABLE;
    packetLocal->e_window = WINDOW_SIZE;
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
    char messToPrint[MESSAGE_BUFF];
    socklen_t len = sizeof(struct sockaddr_in);
    // log file
    FILE * fileToWrite;
    fileToWrite = fopen("sendUDP.txt", "w");

    // packet initialization
    packetLocal->flux_id = 1;
    packetLocal->type = 0;
    packetLocal->seq = 0;
    packetLocal->ack = 0;
    packetLocal->ecn = ECN_DISABLE;
    packetLocal->e_window = 1;

    printf("\n Start stop and wait transmission \n");

    // Envoie d'une série de message
    for (int i = 0; i < 100; ++i) {
        time_t timestamp = time( NULL );
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
                    sprintf(messToPrint, "%ld, %d, false\n", timestamp, packetLocal->seq);
                } else {
                    perror("Erreur au niveau du recvfrom \n");
                    exit(0);
                }
            } else {
                memcpy(packetPertu, messPertu, sizeof(struct packet));
                // if the response is not a timeout, we're done but we need to check if the ack is correct
                if (packetPertu->type == ACK && packetPertu->ack == packetLocal->seq) {
                    printf(" ACK reçu \n");
                    sprintf(messToPrint, "%ld, %d, true\n", timestamp, packetLocal->seq);
                    packetLocal->seq = (packetLocal->seq + 1) % 2;
                } else {
                    printf("Problème d'ack\n");
                }
            }
            fputs(messToPrint, fileToWrite);
        } while (packetPertu->type != ACK);
    }
}

void goBackN(int sock, struct sockaddr_in *socketAdressLocal, struct sockaddr_in *socketAdressPertu) {
    packet packetPertu = malloc(sizeof(struct packet));
    char messPertu[MESSAGE_BUFF];
    char messLocal[MESSAGE_BUFF];
    char messToPrint[MESSAGE_BUFF];
    socklen_t len = sizeof(struct sockaddr_in);
    packet slidingWindows[WINDOW_SIZE] = { NULL };
    int c_window = 3;
    int current_open_slot = c_window;
    int packetNum = 0;
    int packetLocalNum = 0;
    int last_ack = 0;
    int ackAlreadyGet = 0;
    int duplicatedAck = 0;
    struct timeval timeout = {0, 5000};
    int end = FALSE;
    FILE * fileToWrite;
    fileToWrite = fopen("sendUDP.txt", "w");

    // modification du timeout sur la socket afin de simuler du non bloquant
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(struct timeval)) < 0) {
        perror("Problème au niveau du timeout \n");
    }

    printf("\n Start Go Back N transmission \n");
    for (int i = 0; i < WINDOW_SIZE; ++i) {
        slidingWindows[i] = malloc(sizeof(struct packet));
    }
    // Envoie d'une série de message
    do {
        while (current_open_slot > 0) {
            time_t timestamp = time( NULL );
            // packet initialization
            slidingWindows[packetNum]->flux_id = 1;
            slidingWindows[packetNum]->type = 0;
            slidingWindows[packetNum]->seq = packetLocalNum;
            slidingWindows[packetNum]->ack = 0;
            slidingWindows[packetNum]->ecn = ECN_DISABLE;
            slidingWindows[packetNum]->e_window = WINDOW_SIZE;
            // send message
            printf("-------SEND----- \n");
            sprintf(slidingWindows[packetNum]->data, "%s %d %s", "salut ", slidingWindows[packetNum]->seq, " GBN \n");
            printf("%s \n", slidingWindows[packetNum]->data);
            memcpy(messLocal, slidingWindows[packetNum], sizeof(struct packet));
            if (sendto(sock, messLocal, MESSAGE_BUFF, 0, (struct sockaddr *) socketAdressPertu, len) == -1) {
                perror("Erreur au niveau du sendto \n");
                exit(0);
            }
            sprintf(messToPrint, "%ld, %d, send\n", timestamp, packetLocalNum);
            packetLocalNum = packetLocalNum+ 1;
            packetNum = (packetNum + 1) % WINDOW_SIZE;
            current_open_slot--;
            printf("Congestion window : %d | current_open_slot : %d | last_ack : %d \n", c_window, current_open_slot, last_ack);
        }

        if (recvfrom(sock, messPertu, MESSAGE_BUFF, 0, (struct sockaddr *) socketAdressLocal, &len) == -1) {
            if (errno == EWOULDBLOCK) {
                printf("-------Timeout------ \n");
                // division par deux de la congestion window
                flushBuffer(sock, c_window, socketAdressLocal);

                if (c_window % 2 == 0) {
                    c_window = c_window / 2;
                    packetLocalNum = (packetLocalNum - c_window) + 1;
                } else {
                    c_window = (c_window - 1) / 2; // on l'arrondie a l'entier inférieur
                    packetLocalNum = packetLocalNum - c_window;
                }
                current_open_slot = 0;

                // on renvoit les données dans notre fenêtre de congestion
                for (int i = last_ack + 1; i < last_ack + c_window; i++) {
                    printf("%s \n", slidingWindows[i % WINDOW_SIZE]->data);
                    memcpy(messLocal, slidingWindows[i % WINDOW_SIZE], sizeof(struct packet));
                    if (sendto(sock, messLocal, MESSAGE_BUFF, 0, (struct sockaddr *) socketAdressPertu, len) == -1) {
                        perror("Erreur au niveau du sendto \n");
                        exit(0);
                    }
                }
                printf("Congestion window : %d | current_open_slot : %d | last_ack : %d \n", c_window, current_open_slot, last_ack);
            } else {
                perror("Erreur au niveau du recvfrom \n");
                exit(0);
            }
        } else {
            memcpy(packetPertu, messPertu, sizeof(struct packet));
            printf("-------GET ACK------- \n");
            printf("Congestion window : %d | current_open_slot : %d | last_ack : %d \n", c_window, current_open_slot, last_ack);
            printf("%d \n", packetPertu->ack);
            // Si l'ack reçu ne correspond pas au attente et que c'est un ack pas encore reçu, on ack la séquence.
            if ((packetPertu->ack != (last_ack + 1)) && (packetPertu->ack > ackAlreadyGet)) {
                int missingAck = 0;
                if (packetPertu->ack > last_ack) {
                    missingAck = (packetPertu->ack % WINDOW_SIZE) - (last_ack % WINDOW_SIZE);
                } else if (packetPertu->ack < last_ack) {
                    missingAck = ((packetPertu->ack % WINDOW_SIZE) + WINDOW_SIZE) - (last_ack % WINDOW_SIZE);
                }
                // si ça correspond à 0, on le traite normalement
                if (missingAck == 0) {
                    time_t timestamp = time(NULL);
                    if (c_window < (WINDOW_SIZE - 1)) {
                        c_window++;
                        current_open_slot++;
                        sprintf(messToPrint, "%ld, %d, C_UP\n", timestamp, packetPertu->ack);
                        fputs(messToPrint, fileToWrite);
                    }
                    current_open_slot++;
                    ackAlreadyGet++;
                    sprintf(messToPrint, "%ld, %d, ack\n", timestamp, packetPertu->ack);
                    fputs(messToPrint, fileToWrite);
                }
                // si on à une différence, on acquitte la séquence.
                for (int i = 0; i < missingAck; ++i) {
                    time_t timestamp = time(NULL);
                    if (c_window < (WINDOW_SIZE - 1)) {
                        c_window++;
                        sprintf(messToPrint, "%ld, %d, C_UP\n", timestamp, packetPertu->ack);
                        fputs(messToPrint, fileToWrite);
                        current_open_slot++;
                    }
                    current_open_slot++;
                    ackAlreadyGet++;
                    sprintf(messToPrint, "%ld, %d, ack\n", timestamp, ((last_ack + 1) + i));
                    fputs(messToPrint, fileToWrite);
                }
                // si on à une perte d'ack et qu'on nous renvoie le même, on re-send notre fenêtre de congestion.
                // on vide également le buffer afin de ne pas recevoir les anciens acki.
            } else if (packetPertu->ack == last_ack) {
                time_t timestamp = time( NULL );
                printf("wrong ACK \n");
                sprintf(messToPrint, "%ld, %d, WACK\n", timestamp, last_ack);
                fputs(messToPrint, fileToWrite);
                printf("Congestion window : %d | current_open_slot : %d | last_ack : %d \n", c_window, current_open_slot, last_ack);
                flushBuffer(sock, c_window, socketAdressLocal);

                for (int i = last_ack + 1; i < last_ack + c_window; i++) {
                    printf("%s \n", slidingWindows[i % WINDOW_SIZE]->data);
                    memcpy(messLocal, slidingWindows[i % WINDOW_SIZE], sizeof(struct packet));
                    if (sendto(sock, messLocal, MESSAGE_BUFF, 0, (struct sockaddr *) socketAdressPertu, len) == -1) {
                        perror("Erreur au niveau du sendto \n");
                        exit(0);
                    }
                    sprintf(messToPrint, "%ld, %d, Rsend\n", timestamp, slidingWindows[i % WINDOW_SIZE]->seq);
                    fputs(messToPrint, fileToWrite);
                }
            } else if (packetPertu->ack > ackAlreadyGet) {
                time_t timestamp = time(NULL);
                sprintf(messToPrint, "%ld, %d, ack \n", timestamp, packetPertu->ack);
                fputs(messToPrint, fileToWrite);
                if (c_window < (WINDOW_SIZE - 1)) {
                    c_window++;
                    sprintf(messToPrint, "%ld, %d, C_UP\n", timestamp, packetPertu->ack);
                    fputs(messToPrint, fileToWrite);
                    current_open_slot++;
                }
                current_open_slot++;
                ackAlreadyGet++;
            }
            if (packetPertu->ecn > 0) {
                time_t timestamp = time(NULL);
                sprintf(messToPrint, "%ld, %d, ecn \n", timestamp, packetPertu->ack);
                fputs(messToPrint, fileToWrite);
                printf("ECN \n");
                if (c_window % 2 == 0) {
                    (packetLocalNum - (c_window - floor(c_window * 0.9))) + 1;
                } else {
                    packetLocalNum = (packetLocalNum - (c_window - floor(c_window * 0.9)));
                }
                c_window = floor(c_window * 0.9);
                sprintf(messToPrint, "%ld, %d, C_D\n", timestamp, packetPertu->ack);
                fputs(messToPrint, fileToWrite);
                if (current_open_slot != 0) {
                    current_open_slot--;
                }
            }
            if (packetPertu->ack >= ackAlreadyGet) {
                last_ack = packetPertu->ack;
            }
            if (packetPertu->ack >= 1000) {
                end = TRUE;
            }
            printf("Congestion window : %d | current_open_slot : %d | last_ack : %d \n", c_window, current_open_slot, last_ack);
        }
    } while (end == FALSE);
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
    packetLocal->seq = htons(rand() % MAX_SEQ);
    packetLocal->ack = htons(0);
    packetLocal->ecn = ECN_DISABLE;
    packetLocal->e_window = 1;
    sprintf(packetLocal->data, "%s", "FIN");
    memcpy(messLocal, packetLocal, sizeof(struct packet));

    do {
        do {
            // send a the FIN message
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
        } while ((packetPertu->type != ACK) && (packetPertu->ack != packetPertu->seq + 1));
        // waiting the FIN from dest
        if (recvfrom(sock, messPertu, MESSAGE_BUFF, 0, (struct sockaddr *) socketAdressLocal, &len) == -1) {
            if (errno == EWOULDBLOCK) {
                printf("Timeout \n");
            } else {
                perror("Erreur au niveau du recvfrom \n");
                exit(0);
            }
        }
        memcpy(packetPertu, messPertu, sizeof(struct packet));
    } while (packetPertu->type != FIN);

    // send the last ACK
    packetLocal->type = ACK;
    packetLocal->seq = packetPertu->seq;
    packetLocal->ack = packetPertu->ack + 1;
    memcpy(messLocal, packetLocal, sizeof(struct packet));
    if (sendto(sock, messLocal, MESSAGE_BUFF, 0, (struct sockaddr *) socketAdressPertu, len) == -1) {
        perror("Erreur au niveau du sendtod \n");
        exit(0);
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
    timeout.tv_usec = 200*1000; // 0.5 secondes
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

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(struct timeval)) < 0) {
        perror("Problème au niveau du timeout \n");
    }
    // close the connection with a 3way handshake
    endConnection(sock, &socketAdressLocal, &socketAdressPertu);

    return 0;
}


