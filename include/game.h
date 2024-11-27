#ifndef GAME_H
#define GAME_H

#include <arpa/inet.h>
#include <errno.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define UNKNOWN_OPTION_MESSAGE_LEN 24
#define BUFFER_SIZE 1024
#define BASE_TEN 10
#define PACKET_SIZE 256
#define GAME_LOOP_COUNT 5
#define DEFAULT_PORT 8080
#define DEFAULT_IP "192.168.0.1"
#define GAME_GRID_SIZE 100

// Structure for context
typedef struct
{
    int  socket;
    int  localx, localy;
    int  remotex, remotey;
    bool active;
    bool is_host;
    char dataSent[PACKET_SIZE];
    char dataReceived[PACKET_SIZE];
} Context;

// Function prototypes

// Argument handling
void      parse_arguments(int argc, char *argv[], const char **ip_address, char **port_number);
void      handle_arguments(const char *binary_name, const char *ip_address, const char *port_str, in_port_t *port);
in_port_t parse_in_port_t(const char *binary_name, const char *port_str);

// Network setup
int  setupConnection(const int *sockfd, struct sockaddr_storage *addr, const char *ip_address, const char *port);
void convert_address(const char *address, struct sockaddr_storage *addr);
int  socket_create(int domain, int type, int protocol);
void socket_bind(int sockfd, struct sockaddr_storage *addr, in_port_t port);
void socket_close(int sockfd);
void initializeNetwork(const char *ip_address, const char *port);

// Game communication
void    sendUDPMessage(int sockfd, const struct sockaddr_storage *dest_addr, socklen_t addr_len, const char *message);
ssize_t receiveUDPMessage(int sockfd, struct sockaddr_storage *source_addr, socklen_t *addr_len, char *buffer, size_t buffer_size);
void    handleReceivedPacket(const char *packet);

// Game functionality
char *createPacket(int x, int y, const char *game_state);
void  updateRemoteDot(const char *packet);
void  handle_signal(int signal);
// Utility
_Noreturn void usage(const char *program_name, int exit_code, const char *message);
void           generateRandomCoordinates(int *x, int *y);
// void initializeGame();
// void setupNcurses();
// void setStartingPositions();
// void handleInput();
// int  getUserInput();
// void updateLocalDot(int ch);
// void sendPositionUpdate();
// void receivePositionUpdate();
// void updateScreen();
// void clearScreen();
// void drawDot(int x, int y, char symbol);
// void cleanup();
// void errorMessage(const char *msg);

#endif    // GAME_H
