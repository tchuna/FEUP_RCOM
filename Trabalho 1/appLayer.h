#ifndef APPLAYER_H
#define APPLAYER_H

#include "linkLayer.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define BYTESTOSEND 100

typedef enum
{
	TRANSMITTER,
	RECEIVER
} Status;

typedef struct
{
	Status status; /*TRANSMITTER ou RECEIVER */
} ApplicationLayer;

typedef struct
{
	unsigned int type; /* 0 - tamanho do ficheiro, 1 - nome do ficheiro */
	unsigned char lenght; /* tamanho do campo value */
	char *value; /* valor do parâmetro */
} TLV;

typedef struct
{
	unsigned int controlField; /* 2 - start, 3 - end */
	TLV *parameters; /* estrutura declarada em cima */
} ControlPacket;

void startAppLayer(LinkLayer *linkLayer, ApplicationLayer *appLayer);
void transmitter(LinkLayer *linkLayer);
void receiver(LinkLayer *linkLayer);
void send(LinkLayer *linkLayer);
int sendControl(LinkLayer *linkLayer, ControlPacket *controlPacket, int nParameters);
int sendData(LinkLayer *linkLayer, char *buffer, int size, int sequenceNumber);
void receive(LinkLayer *linkLayer);
#endif
