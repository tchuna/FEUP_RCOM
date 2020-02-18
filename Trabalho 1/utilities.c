#include "utilities.h"

volatile int STOP = FALSE;
int CANCEL = FALSE;

int timeOut = TRUE;
int tries = 0;

void alrmHanler(int sig)
{
	printf("alarme # %d\n", tries + 1);
	timeOut = TRUE;
	tries++;
}

void resetTries() { tries = 0; }

void incTries() { tries++; }

int outOfTries(int maxTries) { return tries >= maxTries; }

void setTimeOut(int value)
{
	timeOut = value;
}

int getTimeOut() { return timeOut; }

FILE *openFile(int type, char *filePath)
{
	FILE *result;

	if (type == 0)
		result = fopen(filePath, "wb");

	else
		result = fopen(filePath, "rb");

	if (result == NULL)
	{
		perror("error to open the file ");
		exit(-1);
	}
	return result;
}

void closeFile(FILE *file)
{
	if (fclose(file) != 0)
	{
		perror("closeFile");
		exit(-1);
	}
}

unsigned int getFileSize(char *fileName)
{
	struct stat st;
	if (stat(fileName, &st) < 0)
	{
		perror("getFileSize");
		exit(-1);
	}

	return st.st_size;
}

void sendMessage(int fd, const unsigned char cmd[])
{
	int byteChar = 0;

	while (byteChar != BYTE_TO_SEND)
	{

		byteChar = write(fd, cmd, BYTE_TO_SEND);

		if (byteChar == -1)
		{
			perror("sendMessage");
			exit(-1);
		}
	}
}

int validateCommand(int fd, const unsigned char cmd[])
{
	int state = 0, aux;
	unsigned char reader;

	while (state != FINALSTATE && timeOut == FALSE)
	{

		aux = read(fd, &reader, 1);

		if (aux == -1)
		{
			perror("validateCommand");
			exit(-1);
		}

		switch (state)
		{
		case 0:
			if (reader == cmd[0])
			{
				state = 1;
			}
			else
				state = 0;
			break;
		case 1:
			if (reader == cmd[1])
				state = 2;

			else if (reader != cmd[0])
				state = 0;
			break;
		case 2:
			if (reader == cmd[2])
				state = 3;

			else if (reader != cmd[0])
				state = 0;
			break;
		case 3:

			if ((cmd[3]) == reader)
				state = 4;
			else
				state = 0;
			break;
		case 4:
			if (reader == cmd[4])
				state = 5;

			else
				state = 0;
			break;
		}
	}
	return 0;
}

int validateFrame(int fd, unsigned char *frame)
{

	int state = 0, bytesRead, dataSize = 0;
	unsigned char reader;
	while (state != FINALSTATE && timeOut == FALSE)
	{
		bytesRead = read(fd, &reader, 1);

		if (bytesRead == -1)
		{
			perror("validateFrame");
			exit(-1);
		}

		switch (state)
		{

		case 0: //start
			if (reader == FLAG)
			{
				frame[0] = reader;
				state = 1;
			}
			break;

		case 1: //flag
			if (reader == A)
			{
				frame[1] = reader;
				state = 2;
			}

			else if (reader != FLAG)
				state = 0;
			break;

		case 2: //A

			if (reader == 0 || reader == (1 << 6))
			{
				frame[2] = reader;
				state = 3;
			}

			else if (reader == FLAG)
				state = 1;

			else
				state = 0;
			break;

		case 3: //C
			if ((frame[1] ^ frame[2]) == reader)
			{
				frame[3] = reader;
				state = 4;
			}

			else if (reader == FLAG)
				state = 1;

			else
				state = 0;

			if (simulateError(1))
			{
				state = 0;
				printf("Error simulate\n");
			}
			break;

		case 4:							  //BCC
			frame[4 + dataSize] = reader; //Vai colocando dados até encontrar flag
			dataSize++;
			if (reader == FLAG)
				state = 5;

			break;
		}
	}

	return 4 + dataSize; // F + A + C + BCC1 + dataSize (Dados + BCC2 +)
}

char receiveResponse(int fd)
{
	int state = 0, aux;
	unsigned char reader;
	unsigned char commandReceived;

	while (state != FINALSTATE && timeOut == FALSE)
	{

		aux = read(fd, &reader, 1);

		if (aux == -1)
		{
			perror("receiveResponse");
			exit(-1);
		}

		switch (state)
		{
		case 0:
			if (reader == FLAG)
			{
				state = 1;
			}
			else
				state = 0;
			break;
		case 1:
			if (reader == A)
				state = 2;

			else if (reader != FLAG)
				state = 0;
			break;
		case 2:
			if (reader == C_RR0 ||
				reader == C_RR1 ||
				reader == C_REJ0 ||
				reader == C_REJ1)
			{
				commandReceived = reader;
				state = 3;
			}

			else if (reader != FLAG)
				state = 0;
			break;
		case 3:
			if ((commandReceived ^ A) == reader)
				state = 4;
			else
				state = 0;


			break;
		case 4:
			if (reader == FLAG)
				state = 5;

			else
				state = 0;
			break;
		}
	}

	if (state == FINALSTATE)
		return commandReceived;

	return 0;
}

int stuffing(unsigned char *frame, int size)
{
	char *result = malloc(20000);
	int resultSize = size;
	int i;

	for (i = 1; i < (size - 1); i++)
	{
		if (frame[i] == FLAG || frame[i] == ESC)
		{
			resultSize++;
		}
	}
	result[0] = frame[0];
	int j = 1;

	for (i = 1; i < (size - 1); i++)
	{
		if (frame[i] == FLAG || frame[i] == ESC)
		{
			result[j] = ESC;
			result[++j] = frame[i] ^ 0X20;
		}
		else
		{
			result[j] = frame[i];
		}
		j++;
	}

	result[j] = frame[i];
	frame = realloc(frame, size + resultSize - size);
	memcpy(frame, result, resultSize);
	return resultSize;
}

int destuffing(unsigned char *frame, int size)
{
	int i, j = 0;
	char *result = malloc(20000);
	int resultSize = size;

	for (i = 1; i < (size - 1); i++)
	{
		if (frame[i] == ESC)
			resultSize--;
	}

	for (i = 0; i < size; i++)
	{
		if (frame[i] == ESC)
			result[j] = frame[++i] ^ 0X20;

		else
			result[j] = frame[i];

		j++;
	}
	memcpy(frame, result, resultSize);

	return resultSize;
}

int isValidBcc2(unsigned char *packet, int packetSize, unsigned char received)
{
	unsigned char expected = 0;

	unsigned int i = 4;
	for (; i < packetSize - 2; i++)
	{
		expected ^= packet[i];
	}

	if (simulateError(2))
	{
		printf("Error simulate2\n");
		return 0;
	}

	return (expected == received);
}

int simulateError (int value)
{
	int n = rand() % 100;

	if (value == 1)
		return n < ERRORPROBABILITY1;
	if (value == 2)
		return n < ERRORPROBABILITY2;

	return 0;
}

int validBaudRate(int brate){
	int speed;
	switch(brate){
	case 1: speed=B600;
        break;
	case 2: speed=B1200;
        break;
	case 3: speed=B1800;
        break;
	case 4: speed=B2400;
        break;
	case 5: speed=B4800;
        break;
	case 6: speed=B9600;
        break;
	case 7: speed=B19200;
        break;
	case 8: speed=B38400;
        break;

	}
return speed;

}
