#ifndef LINKLAYER_H
#define LINKLAYER_H

#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <termios.h>
#include <strings.h>
#include <signal.h>
#include <time.h>
#include "utilities.h"

#define BAUDRATE B38400

typedef struct
{
	int fd;						   /*Descritor de ficheiro */
	char *port;					   /*Dispositivo /dev/ttySx, x = 0, 1*/
	int baudRate;				   /*Velocidade de transmissão*/
	unsigned int sequenceNumber;   /*Número de sequência da trama: 0, 1*/
	unsigned int timeout;		   /*Valor do temporizador:*/
	unsigned int numTransmissions; /*Número de tentativas em caso de falha*/
	unsigned char *frame;		   /*Trama */

	char *fileName; /*nome do ficheiro */
	int fileSize; /*tamanho do ficheiro */

	unsigned int nRR; /*número de RRs enviados/recebidos */
	unsigned int nREJ; /*número de REJs enviados/recebidos */
	double totalTime; /*tempo decorrido durante a transmissão do ficheiro */
} LinkLayer;

struct termios oldtio, newtio;

void setupLinkLayer(LinkLayer *linkLayer,int brate,char* port,char* filename);
int openPort(LinkLayer *linkLayer);
int setTermiosStructure(LinkLayer *linkLayer);
int llopenT(LinkLayer *linkLayer);
int llopenR(LinkLayer *linkLayer); 
int llwrite(LinkLayer *linkLayer, unsigned char *buffer, int lenght);
int llread(LinkLayer *linkLayer);
int llcloseT(LinkLayer *linkLayer); 
int llcloseR(LinkLayer *linkLayer); 

#endif
