#ifndef UTILITIES_H
#define UTILITIES_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#define MAX_SIZE 255 
#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define ERRORPROBABILITY1 0
#define ERRORPROBABILITY2 0
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define FLAG 0x7E
#define ESC 0x7D
#define FLAG_S_2 0x5E
#define FLAG_S_3 0x5D

#define C_REJ0 0X01
#define C_REJ1 0X81

#define C_RR0 0X05
#define C_RR1 0X85

#define A 0x03
#define C_SET 0x03
#define C_DISC 0x0B
#define C_UA 0x07
#define BYTE_TO_SEND 5
#define FINALSTATE 5

static const unsigned char SETUP[5] = { FLAG, A, C_SET, A ^ C_SET, FLAG };
static const unsigned char UA[5] = { FLAG, A, C_UA, A ^ C_UA, FLAG };
static const unsigned char DISC[5] = { FLAG, A, C_DISC, A ^ C_DISC, FLAG };
static const unsigned char RR0[5] = { FLAG, A, C_RR0, A ^ C_RR0, FLAG };
static const unsigned char RR1[5] = { FLAG, A, C_RR1, A ^ C_RR1, FLAG };
static const unsigned char REJ0[5] = { FLAG, A, C_REJ0, A ^ C_REJ0, FLAG };
static const unsigned char REJ1[5] = { FLAG, A, C_REJ1, A ^ C_REJ1, FLAG };



void alrmHanler(int sig);
void resetTries();
void incTries();
int outOfTries (int maxTries);
void setTimeOut (int value);
int getTimeOut ();
FILE* openFile(int type ,char* filePath);
void closeFile(FILE * file);
unsigned int getFileSize(char *fileName);
void sendMessage(int fd, const unsigned char cmd[]);
int validateCommand(int fd, const unsigned char cmd[]);
int validateFrame(int fd, unsigned char * frame);
char receiveResponse(int fd);
int stuffing(unsigned char* frame,int size);
int destuffing(unsigned char* frame,int size);
int isValidBcc2(unsigned char * packet,int packetSize,unsigned char received);
int simulateError();
int validBaudRate(int brate);

#endif
