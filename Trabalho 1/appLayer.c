#include "appLayer.h"

void startAppLayer(LinkLayer *linkLayer, ApplicationLayer *appLayer)
{
	if (openPort(linkLayer) < 0)
		exit(-1);

	else if (setTermiosStructure(linkLayer) < 0)
		exit(-1);

	switch (appLayer->status)
	{
	case TRANSMITTER:
		transmitter(linkLayer);
		break;
	case RECEIVER:
		receiver(linkLayer);
		break;
	}
}

void transmitter(LinkLayer *linkLayer)
{
	if (llopenT(linkLayer) < 0)
		exit(-1);

	send(linkLayer);

	if (llcloseT(linkLayer) < 0)
		exit(-1);
}

void receiver(LinkLayer *linkLayer)
{
	if (llopenR(linkLayer) < 0)
		exit(-1);

	receive(linkLayer);

	if (llcloseR(linkLayer) < 0)
		exit(-1);
}

void send(LinkLayer *linkLayer)
{
	char sizeString[16];

	//Start control packet
	ControlPacket startCP;
	startCP.controlField = 2;

	//control packet name
	TLV startTLVName;

	startTLVName.type = 1;
	startTLVName.lenght = strlen(linkLayer->fileName);
	startTLVName.value = linkLayer->fileName;

	//control packet size
	TLV startTLVSize;

	linkLayer->fileSize = getFileSize(linkLayer->fileName);

	sprintf(sizeString, "%d", linkLayer->fileSize);

	startTLVSize.type = 0;
	startTLVSize.lenght = strlen(sizeString);
	startTLVSize.value = sizeString;

	TLV listParameters[2] = {startTLVSize, startTLVName};
	startCP.parameters = listParameters;

	struct timeval start;
	gettimeofday(&start, NULL);

	sendControl(linkLayer, &startCP, 2);
	printf("Start control packet sent\n");

	//Data packet

	FILE *file = openFile(1, linkLayer->fileName);

	char *fileData = (char *)malloc(linkLayer->fileSize);
	int nBytesRead = 0, sequenceNumber = 0;

	printf("Sending data\n");
	while ((nBytesRead = fread(fileData, sizeof(unsigned char), BYTESTOSEND, file)) > 0) 
	{
		sendData(linkLayer, fileData, BYTESTOSEND, sequenceNumber++ % 255);

		memset(fileData, 0, BYTESTOSEND);
	}

	free(fileData);
	closeFile(file);

	//End control packet
	ControlPacket endCP;
	endCP = startCP;
	endCP.controlField = 3;

	tcflush(linkLayer->fd, TCIOFLUSH);

	sendControl(linkLayer, &endCP, 2);
	printf("End control packet sent\n");

	struct timeval end;
	gettimeofday(&end, NULL);


	linkLayer->totalTime = (double) (end.tv_usec - start.tv_usec) / 1000000 + (double) (end.tv_sec - start.tv_sec);
}

int sendControl(LinkLayer *linkLayer, ControlPacket *controlPacket, int nParameters)
{
	unsigned int i, packetSize = 1, index = 1;

	for (i = 0; i < nParameters; i++)
		packetSize += 2 + controlPacket->parameters[i].lenght;

	unsigned char frame[packetSize];
	frame[0] = controlPacket->controlField;

	for (i = 0; i < nParameters; i++)
	{
		frame[index++] = controlPacket->parameters[i].type;
		frame[index++] = controlPacket->parameters[i].lenght;
		memcpy(&frame[index], controlPacket->parameters[i].value, controlPacket->parameters[i].lenght);
		index += controlPacket->parameters[i].lenght;
	}

	if (llwrite(linkLayer, frame, packetSize) < 0)
		exit(-1);

	linkLayer->sequenceNumber = !linkLayer->sequenceNumber;

	return 0;
}

int sendData(LinkLayer *linkLayer, char *buffer, int size, int sequenceNumber)
{
	unsigned char L1, L2;
	unsigned int packetSize;

	//usando teorema do resto
	L1 = size % 256;
	L2 = size / 256;

	packetSize = 4 + size;
	unsigned char *frame = malloc(packetSize);

	frame[0] = 1;
	frame[1] = sequenceNumber;
	frame[2] = L2;
	frame[3] = L1;

	memcpy(&frame[4], buffer, size);

	if (llwrite(linkLayer, frame, packetSize) < 0)
		exit(-1);

	linkLayer->sequenceNumber = !linkLayer->sequenceNumber;

	return 0;
}

void receive(LinkLayer *linkLayer)
{
	int size;
	unsigned int fileSize, index;
	char *fileName;
	struct timeval start;
	gettimeofday(&start, NULL);
	//Start control packet
	
	do
	{
		size = llread(linkLayer);
	} while(linkLayer->frame[4] != 2);

	index = 0;

	while (index < size)
	{
		unsigned int type = linkLayer->frame[index++];	//0 = size, 1 = name
		unsigned char lenght = linkLayer->frame[index++]; //size of file
		char *value = malloc(lenght);					  // either size or name, according to type

		memcpy(value, &linkLayer->frame[index], lenght);

		if (type == 0) //stores size of file in fileSize
		{
			fileSize = atoi(value);
		}

		else if (type == 1) //stores name of file in fileNAme
		{
			fileName = malloc(lenght);
			memcpy(fileName, value, lenght);
		}

		index += lenght;
	}

	printf("Received start control packet.\n");

	//DAQUI PARA BAIXO LÊ OS DADOS ATÉ RECEBER CONTROL PACKET A INDICAR FIM

	linkLayer->fileName = fileName;
	linkLayer->fileSize = fileSize;

	FILE *file = openFile(0, "e.gif"); 

	printf("Receiving data\n");

	while (1)
	{
		char *data;
		int dataC;
		unsigned int L1, L2, lenght;

		size = llread(linkLayer);

		if (size < 0)
			continue;

		// frame[0] = FLAG
		// frame[1] = A
		// frame[2] = C da trama, não dos dados
		// frame[3] = BBC1
		// A partir do 4 começa dos dados
		dataC = linkLayer->frame[4];

		if (dataC == 3) //receives end control packet
		{
			printf("Received end control packet.\n");
			break;
		}

		else if (dataC != 1)
		{
			linkLayer->nREJ++;
			if (linkLayer->sequenceNumber)
				sendMessage(linkLayer->fd, REJ1);
			else
				sendMessage(linkLayer->fd, REJ0);

			continue;
		}

		L2 = linkLayer->frame[6];
		L1 = linkLayer->frame[7];

		lenght = 256 * L2 + L1;

		data = malloc(lenght);

		memcpy(data, &linkLayer->frame[8], lenght);

		fwrite(data, sizeof(char), lenght, file);
		free(data);

	}

	closeFile(file);

	struct timeval end;
	gettimeofday(&end, NULL);

	linkLayer->totalTime = (double) (end.tv_usec - start.tv_usec) / 1000000 + (double) (end.tv_sec - start.tv_sec);
}
