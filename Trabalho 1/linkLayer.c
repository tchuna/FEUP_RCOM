#include "linkLayer.h"
#include <stdio.h>

void setupLinkLayer(LinkLayer *linkLayer, int baudrate, char *port, char *filename)
{
    linkLayer->fd = -1;
    linkLayer->port = port;
    linkLayer->baudRate = validBaudRate(baudrate);
    linkLayer->sequenceNumber = 0;
    linkLayer->timeout = 3;
    linkLayer->numTransmissions = 3;
    linkLayer->frame = malloc(MAX_SIZE);

    linkLayer->fileName = filename;
    linkLayer->fileSize = 0;

    linkLayer->nRR = 0;
    linkLayer->nREJ = 0;
    linkLayer->totalTime = 0;
}

int openPort(LinkLayer *linkLayer)
{
    linkLayer->fd = open(linkLayer->port, O_RDWR | O_NOCTTY);

    if (linkLayer->fd == -1)
    {
        perror("openPort");
        return -1;
    }

    return 0;
}

int setTermiosStructure(LinkLayer *linkLayer)
{
    if (tcgetattr(linkLayer->fd, &oldtio) == -1)
    { /* save current port settings */
        perror("setTermiosStructure");
        return -1;
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = linkLayer->baudRate | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    newtio.c_lflag = 0;

    newtio.c_cc[VTIME] = 1; /* inter-character timer unused */
    newtio.c_cc[VMIN] = 0;  /* blocking read until 5 chars received */

    tcflush(linkLayer->fd, TCIOFLUSH);

    if (tcsetattr(linkLayer->fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        return -1;
    }

    return 0;
}

int llopenT(LinkLayer *linkLayer)
{

    (void)signal(SIGALRM, alrmHanler);

    while (!outOfTries(linkLayer->numTransmissions) && getTimeOut() == TRUE)
    {
        setTimeOut(FALSE);

        alarm(linkLayer->timeout);

        sendMessage(linkLayer->fd, SETUP);
        validateCommand(linkLayer->fd, UA);

        alarm(0);
    }

    if (outOfTries(linkLayer->numTransmissions))
    {
        printf("Failed to send the message (%d attemps)\n", linkLayer->numTransmissions);
        return -1;
    }

    return 0;
}

int llopenR(LinkLayer *linkLayer)
{
    setTimeOut(FALSE);

    validateCommand(linkLayer->fd, SETUP);
    sendMessage(linkLayer->fd, UA);

    return 0;
}

int llwrite(LinkLayer *linkLayer, unsigned char *buffer, int lenght)
{

    unsigned char *packet = malloc(12000);
    resetTries();
    setTimeOut(TRUE);

    packet[0] = FLAG;
    packet[1] = A;
    packet[2] = linkLayer->sequenceNumber << 6;
    packet[3] = packet[1] ^ packet[2];

    memcpy(&packet[4], buffer, lenght);

    unsigned char BCC2 = 0;
    int i = 0;

    for (; i < lenght; i++)
        BCC2 ^= buffer[i];

    packet[lenght + 4] = BCC2;
    packet[lenght + 5] = FLAG;

    int newLenght = stuffing(packet, lenght + 6);

    while (!outOfTries(linkLayer->numTransmissions) && getTimeOut() == TRUE)
    {
        setTimeOut(FALSE);
        alarm(linkLayer->timeout);

        if (write(linkLayer->fd, packet, newLenght + 6) < 0)
        {
            perror("write");
            return -1;
        }

        unsigned char response = 10;
        response = receiveResponse(linkLayer->fd);

        if (response == C_RR0 ||
            response == C_RR1)
            linkLayer->nRR++;

        else if (response == C_REJ0 ||
                 response == C_REJ1)
        {
            setTimeOut(TRUE);
            incTries();
            linkLayer->nREJ++;
        }

        alarm(0);
    }

    if (outOfTries(linkLayer->numTransmissions))
    {
        printf("Failed to send the message (%d attemps)\n", linkLayer->numTransmissions);
        exit(-1);
    }

    return 0;
}

int llread(LinkLayer *linkLayer)
{
    setTimeOut(FALSE);

    int size = validateFrame(linkLayer->fd, linkLayer->frame);

    size = destuffing(linkLayer->frame, size);

    if (linkLayer->sequenceNumber != linkLayer->frame[2] >> 6 ||
        !isValidBcc2(linkLayer->frame, size, linkLayer->frame[size - 2]))
    {
        linkLayer->nREJ++;
        size = -1;

        if (linkLayer->sequenceNumber)
            sendMessage(linkLayer->fd, REJ1);
        else
            sendMessage(linkLayer->fd, REJ0);
    }

    else
    {
        linkLayer->nRR++;

        if (linkLayer->sequenceNumber)
            sendMessage(linkLayer->fd, RR1);
        else
            sendMessage(linkLayer->fd, RR0);

        linkLayer->sequenceNumber = !linkLayer->sequenceNumber;
    }

    return size;
}

int llcloseT(LinkLayer *linkLayer)
{
    resetTries();
    setTimeOut(TRUE);

    while (!outOfTries(linkLayer->numTransmissions) && getTimeOut() == TRUE)
    {

        setTimeOut(FALSE);
        alarm(linkLayer->timeout);

        sendMessage(linkLayer->fd, DISC);

        validateCommand(linkLayer->fd, DISC);

        alarm(0);
    }

    if (outOfTries(linkLayer->numTransmissions))
    {
        printf("Failed to send the message (%d attemps)\n", linkLayer->numTransmissions);
        return -1;
    }

    sendMessage(linkLayer->fd, UA);

    return 0;
}

int llcloseR(LinkLayer *linkLayer)
{
    resetTries();
    setTimeOut(TRUE);

    (void)signal(SIGALRM, alrmHanler);

    validateCommand(linkLayer->fd, DISC);

    while (!outOfTries(linkLayer->numTransmissions) && getTimeOut() == TRUE)
    {

        setTimeOut(FALSE);
        alarm(linkLayer->timeout);

        sendMessage(linkLayer->fd, DISC);

        validateCommand(linkLayer->fd, UA);

        alarm(0);
    }

    if (outOfTries(linkLayer->numTransmissions))
    {
        printf("Failed to send the message (%d attemps)\n", linkLayer->numTransmissions);
        return -1;
    }

    return 0;
}
