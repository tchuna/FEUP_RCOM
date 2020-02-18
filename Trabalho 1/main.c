#include "linkLayer.h"
#include "appLayer.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

void printStats (LinkLayer * linkLayer)
{
	printf("\nFile name = %s\n", linkLayer->fileName);
	printf("File size = %d\n", linkLayer->fileSize);
	printf("#RR = %d \n#REJ = %d\n", linkLayer->nRR, linkLayer->nREJ);
	printf("Time: %f s\n\n", linkLayer->totalTime);
}




int  printChose()
{
	int bChose;
	printf("Chose the Baudrate to transmit the File:");
	printf("\n");
	printf("1->600   2->1200  3->1800   4->2400 ");
	printf("\n");
	printf("5->4800  6->9600  7->19200  8->38400 ");
	printf("\n\n");

	printf("Enter the value:");
	scanf("%d", &bChose);

	while(bChose<1 || bChose>8){
		printf("Invalid value\n");
		printf("Enter the value:");
		scanf("%d", &bChose);

	}
	return bChose;
}

int main(int argc, char *argv[]) {

	srand ( time(NULL) );
	int resultB;


	LinkLayer * linkLayer = malloc(sizeof(LinkLayer));


	if ( (argc != 4) ||
	     ((strcmp("/dev/ttyS0", argv[1])!=0) &&
	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
	   printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS0 mode filename\n");
	   exit(1);
	 }


	resultB=printChose();
	setupLinkLayer (linkLayer,resultB,argv[1],argv[3]);

	ApplicationLayer * appLayer = malloc(sizeof(ApplicationLayer));

	if ((strcmp("0", argv[2]) == 0)){
		appLayer->status = TRANSMITTER;

	}
	else if ((strcmp("1", argv[2]) == 0))
		appLayer->status = RECEIVER;
	else
	{
		printf("Mode must be either 0 (TRANSMITTER) or 1 (RECEIVER)\n.");
		return -1;
	}

	startAppLayer(linkLayer, appLayer);
	printStats(linkLayer);
	return 0;
}
