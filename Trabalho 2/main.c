#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <strings.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <ctype.h>

#define SERVER_PORT 21
#define SERVER_ADDR "192.168.28.96"
#define MAX_SIZE 1000
#define PASSWORD_RESPONSE "230" //User logged in, proceed.
#define USER_RESPONSE "331" //User name okay, need password.
#define CWD_RESPONSE "250" //Requested file action okay, completed.


typedef struct
{
    char *user;
    char *password;
    char *host;
    char *path;
    char *filename;
    char *ip;
} URL;

void initURL(URL *url);
int parseURL(char *arg, URL *url);
void showURLInfo(URL *url);
void getIp(URL *url);
char * receiveResponse(int sockedfd);
void login(int socketfd, URL *url);
int handleResponse(int socketfd, char * cmd, int socketfd2, URL *url);
int changeDir(int socketfd, URL * url);
int passiveMode(int socketfd, URL * url);
int getPort(int socketfd, URL * url);
void retrieve(int socketfd, int socketfd2, URL *url);
void fileCreation(int socketfd, char * filename);

int main(int argc, char **argv)
{
    int sockfd, sockfd2;
    struct sockaddr_in server_addr, server_addr2;

    if (argc != 2)
    {
        printf("Usage: %s ftp://[<user>:<password>@]<host>/<url-path>\n", argv[0]);
        exit(-1);
    }

    URL url;
    initURL(&url);

    parseURL(argv[1], &url);
    getIp(&url);
    showURLInfo(&url);

    /*server address handling*/
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(url.ip); /*32 bit Internet address network byte ordered*/
    server_addr.sin_port = htons(SERVER_PORT);            /*server TCP port must be network byte ordered */

    /*open an TCP socket*/
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket()");
        exit(0);
    }

    /*connect to the server*/
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("connect()");
        exit(0);
    }

    receiveResponse(sockfd);

    login(sockfd, &url);

    changeDir(sockfd, &url);

    int port = passiveMode(sockfd, &url);

    //server address handling
    bzero((char *)&server_addr2, sizeof(server_addr2));
    server_addr2.sin_family = AF_INET;
    server_addr2.sin_addr.s_addr = inet_addr(url.ip); //32 bit Internet address network byte ordered
    server_addr2.sin_port = htons(port); //server TCP port must be network byte ordered

    //open an TCP socket
    if ((sockfd2 = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket()");
        exit(0);
    }
    //connect to the server
    if (connect(sockfd2, (struct sockaddr *)&server_addr2, sizeof(server_addr2)) < 0)
    {
        perror("connect()");
        exit(0);
    }
    retrieve(sockfd, sockfd2, &url);

    printf("File retrieved. Program will now exit.\n");

    close(sockfd);
    close(sockfd2);

    exit(0);
}

void initURL(URL *url)
{
    url->user = malloc(MAX_SIZE);
    memset(url->user, 0, MAX_SIZE);

    url->password = malloc(MAX_SIZE);
    memset(url->password, 0, MAX_SIZE);

    url->host = malloc(MAX_SIZE);
    memset(url->host, 0, MAX_SIZE);

    url->path = malloc(MAX_SIZE);
    memset(url->path, 0, MAX_SIZE);

    url->filename = malloc(MAX_SIZE);
    memset(url->filename, 0, MAX_SIZE);

    url->ip = malloc(MAX_SIZE);
    memset(url->ip, 0, MAX_SIZE);

}

int parseURL(char *arg, URL *url)
{
    char prefix[7] = "ftp://";
    unsigned int length = strlen(arg), state = 0, index = 0, i = 0;

    while (index < length)
    {
        switch (state)
        {
        //Prefix
        case 0:
        {

            if (arg[index] == prefix[index])
            {
                if (index < 5) //Prefix is still being read
                    break;

                else if (index == 5) //Prefix was read correctly
                    state = 1;
            }

            else
            {
                printf("Error reading ftp://\n");
                exit(1);
            }

            break;
        }

        //user
        case 1:
        {

            if (arg[index] == '/') //No username and password specified. User will be considered as "anonymous"
            {
                i = 0;
                url->host = url->user;
                url->user = "anonymous";
                state = 4;
                printf("1\n");
            }

            else if (arg[index] == ':') //Finished
            {
                i = 0;
                state = 2;
            }

            else
            {
                url->user[i] = arg[index];
                i++;
            }

            break;
        }

        //password
        case 2:
        {
            if (arg[index] == '@') //Finished
            {
                i = 0;
                state = 3;
            }

            else
            {
                url->password[i] = arg[index];
                i++;
            }

            break;
        }

        //host
        case 3:
        {
            if (arg[index] == '/') //Finished
            {
                i = 0;
                state = 4;
            }

            else
            {
                url->host[i] = arg[index];
                i++;
            }

            break;
        }

        //path
        case 4:
        {
            url->path[i] = arg[index];
            i++;

            break;
        }
        }

        index++;
    }


    //filename is going to be the string after the last '/' in path
    unsigned int indexPath = 0, indexFilename = 0;

    char * tmp = malloc(MAX_SIZE);
    memset(tmp, 0, MAX_SIZE);

    for (indexPath = 0; indexPath < strlen(url->path); indexPath++)
    {
        if (url->path[indexPath] == '/') // new directory
        {
            strcat(tmp, "/");
            strcat(tmp, url->filename);
            indexFilename = 0;
            memset(url->filename, 0, MAX_SIZE); //resets filename
            continue;
        }

        url->filename[indexFilename] = url->path[indexPath];
        indexFilename++;
    }

    url->path = tmp;

    return 0;
}

void showURLInfo(URL *url)
{
    printf("\nUser: %s\n", url->user);
    printf("Password: %s\n", url->password);
    printf("Host: %s\n", url->host);
    printf("Ip address: %s\n", url->ip);
    printf("Path: %s/%s\n", url->path, url->filename);
    printf("Filename: %s\n\n", url->filename);
}

void getIp(URL *url)
{
    struct hostent *h;

    if ((h = gethostbyname(url->host)) == NULL)
    {
        herror("gethostbyname");
        exit(1);
    }

    url->ip = inet_ntoa(*((struct in_addr *)h->h_addr));
}

char * receiveResponse(int sockedfd)
{
    /* Example:
    123- First line
    Second line
    234 A line beginning with numbers
    123 The last line*/

    unsigned int state = 0, index = 0;
    char reader;
    char * code = malloc(3);

    while (state < 3)
    {
        read(sockedfd, &reader, 1);
        printf("%c", reader);

        switch(state)
        {
            //3 digits code followed by space(one line) or hiphen(multi line) (RFC959 p.35)
            case 0:
            {
                //one line response
                if(reader == ' ')
                {
                    if(index != 3)
                    {
                        printf("Error reading response's 3-digit code\n");
                        exit(1);
                    }

                    index = 0;
                    state = 1;
                }

                //multi line
                else if (reader == '-')
                {
                    index = 0;
                    state = 2;
                }

                //code still not finished
                else if(isdigit(reader))
                {
                    code[index] = reader;
                    index++;
                }
                break;
            }

            //Last line is being read
            case 1:
            {
                //If line ends, state machine also ends
                if(reader == '\n')
                    state = 3;
                break;
            }

            //Multi line codes
            case 2:
            {
                //Since response code was found, that means that this is the last line
                if(index == 3 && reader == ' ')
                    state = 1;

                //Code being read can be the response code
                else if(reader == code[index])
                    index++;

                //Response code was found but the line is not the last one
                else if(index == 3 && reader == '-')
                    index = 0;

                break;
            }
        }
    }

    return code;
}

void login(int socketfd, URL *url)
{
    char * user = malloc(strlen(url->user)+6), * password = malloc(strlen(url->password)+6);

    sprintf(user, "USER %s\n", url->user);
    sprintf(password, "PASS %s\n", url->password);

    if(write(socketfd, user, strlen(user)) < 0)
    {
        perror("Sending user");
        exit(1);
    }

    if (handleResponse(socketfd, user, 0, url) == 3)
    {
	    if(write(socketfd, password, strlen(password)) < 0)
	    {
      		perror("Sending password");
      		exit(1);
	    }

	    handleResponse(socketfd, password, 0, url);
    }
}

int handleResponse(int socketfd, char * cmd, int socketfd2, URL *url)
{
    char * responseCode = malloc(3);
    responseCode = receiveResponse(socketfd);

    while(1)
    {

        switch(responseCode[0])
        {

            //The requested action is being initiated; expect another reply before proceeding with a new command.
            case '1':
            {
              if (cmd[0] == 'R' &&
                  cmd[1] == 'E' &&
                  cmd[2] == 'T' &&
                  cmd[3] == 'R')
                  fileCreation(socketfd2, url->filename);

                responseCode = receiveResponse(socketfd);
                break;
            }
            //The requested action has been successfully completed
            case '2':
            {
                return 2;
            }
            // The command has been accepted, but the requested action
            // is being held in abeyance, pending receipt of further
            // information.  The user should send another command
            // specifying this information. (probably asking for password)
            case '3':
            {
                return 3;
            }
            //The command was not accepted and the requested action did
            //   not take place, but the error condition is temporary and
            //   the action may be requested again.
            case '4':
            {
                write(socketfd, cmd, strlen(cmd));
                responseCode = receiveResponse(socketfd);
                break;
            }
            //The command was not accepted and the requested action did
            //not take place.  The User-process is discouraged from
            //repeating the exact request (in the same sequence).
            case '5':
            {
                printf("%s not accepted.", cmd);
                close(socketfd);
                exit(1);
            }
        }
    }


}

int changeDir(int socketfd, URL * url)
{
    char * cwd = malloc(strlen(url->user)+5);
    sprintf(cwd, "CWD %s\n", url->path);

    if(write(socketfd, cwd, strlen(cwd)) < 0)
    {
        perror("Changing directory");
        exit(1);
    }

    handleResponse(socketfd, cwd, 0, url);

    return 0;
}

int passiveMode(int socketfd, URL * url)
{
    int port;

    if(write(socketfd, "PASV\n", strlen("PASV\n")) < 0)
    {
        perror("Entering passive mode");
        exit(1);
    }

    port = getPort(socketfd, url);

    return port;
}

int getPort(int socketfd, URL * url)
{
    //Example: 227 Entering Passive Mode (A1,A2,A3,A4,a1,a2)

    char prefix[26] = "227 Entering Passive Mode ";
    unsigned int length = 26, index = 0, A1 = 0, A2 = 0, A3 = 0, A4 = 0, a1 = 0, a2 = 0, counter = 1;
    char reader;
    //checks if prefix is ok
    while(index < length)
    {
        read(socketfd, &reader, 1);
        printf("%c", reader);

        if(prefix[index] == reader)
        {
            index++;
            continue;
        }

    	else if(prefix[0] == reader && index != 0 && index !=1)
        {
            index = 2;
            continue;
        }

        else
            index = 0;

    }
    //Gets port a1,a1
    do
    {

        read(socketfd, &reader, 1);
        printf("%c", reader);

        //Finished reading a number
        if(reader == ',')
            counter++;

        if(isdigit(reader))
        {
            unsigned int converted = reader - '0';

            switch(counter)
            {
                case 1:
                    A1 = A1 * 10 + converted;
                    break;
                case 2:
                    A2 = A2 * 10 + converted;
                    break;
                case 3:
                    A3 = A3 * 10 + converted;
                    break;
                case 4:
                    A4 = A4 * 10 + converted;
                    break;
                case 5:
                    a1 = a1 * 10 + converted;
                    break;
                case 6:
                    a2 = a2 * 10 + converted;
                    break;

            }
        }
    }while(reader != ')');

    sprintf(url->ip,"%d.%d.%d.%d", A1, A2, A3, A4);
    return a1 * 256 + a2;
}

void retrieve(int socketfd, int socketfd2, URL *url)
{
    char * retrieve = malloc(strlen(url->filename)+6);
    sprintf(retrieve, "RETR %s\n", url->filename);

    if(write(socketfd, retrieve, strlen(retrieve)) < 0)
    {
        perror("Retrieving");
        exit(1);
    }

    handleResponse(socketfd, retrieve, socketfd2, url);
}

void fileCreation(int socketfd, char * filename)
{
    FILE *file;
    char buffer[1024];
    int bytesRead;

    if ((file = fopen(filename, "wb+")) == NULL)
    {
    	perror("Creating file");
    	exit(1);
    }

    while ((bytesRead = read(socketfd, buffer, 1024)) > 0) {
    	bytesRead = fwrite(buffer, bytesRead, 1, file);
    }
}
