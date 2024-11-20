#ifndef GAME_H
#define GAME_H

#include <stdbool.h>

// Structure for context
typedef struct {
    int socket;
    int localx, localy;
    int remotex, remotey;
    bool active;
    char dataSent[256];
    char dataReceived[256];
} Context;

// Function prototypes
void initializeGame();
void setupNcurses();
void initializeNetwork();
void setStartingPositions();
void handleInput();
int getUserInput();
void updateLocalDot(int ch);
void sendPositionUpdate();
void createPacket(int x, int y);
void sendUDPMessage(const char* packet);
void receivePositionUpdate();
void receiveUDPMessage();
void updateRemoteDot(const char* packet);
void updateScreen();
void clearScreen();
void drawDot(int x, int y, char symbol);
void cleanup();
void errorMessage(const char* msg);

#endif // GAME_H
