/*
 * client.c
 */

#include <stdio.h>
#include <iostream>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstdlib>
#include <cstring>

using namespace std;

#define SERVER_PORT 2403
#define MAX_LINE 256

//Description: makes all characters lower case
void to_lower_buf(char* buf, int num_chars)
{
    for (int i = 0; i < num_chars; ++i)
    {
        buf[i] = tolower(buf[i]);
    }
}

//Description: returns true if string has a space
bool has_space(string str)
{
    for (int i = 0; i < str.size(); ++i)
    {
        if (isspace(str.at(i)))
        {
            return true;
        }
    }

    return false;
}

//Main
int main(int argc, char * argv[]) 
{

    struct sockaddr_in sin;
    char buf[MAX_LINE];
    char rbuf[MAX_LINE];
    int len;
    int s;

    if (argc < 2) 
    {
		cout << "Usage: client <Server IP Address>" << endl;
		exit(1);
    }

    /* active open */
    if ((s = socket (AF_INET, SOCK_STREAM, 0)) < 0) 
    {
		perror("socket");
		exit(1);
    }

    /* build address data structure */
    bzero((char *)&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr  = inet_addr(argv[1]);
    sin.sin_port = htons (SERVER_PORT);

    if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		perror("connect");
		close(s);
		exit(1);
    }

    cout << "You have entered the YAMOTD program. Please enter a command.\nc: ";

    /* main loop; get and send lines of text */
    while (fgets(buf, sizeof(buf), stdin)) 
    {
        /* make sure character buf has null character */
        buf[MAX_LINE - 1] = '\0';

        /* make all characters in buf lower case */
        to_lower_buf(buf, strlen(buf));

        /* calculate length of message */       
        len = strlen(buf) + 1;

        /* to determine if the user is trying to login */ 
        string first_word(buf);

        if (has_space(first_word))
        {
            /* just get first word of input */
            first_word = first_word.substr(0, first_word.find(' '));
        }

        if (strcmp(buf, "msgget\n") == 0)
        {
            /* send command */
            send(s, buf, len, 0);

            /* wait for message of the day */
            recv(s, rbuf, sizeof(rbuf), 0);

            /* print message of the day */
            cout << rbuf;
        }

        else if (strcmp(buf, "quit\n") == 0)
        {
            /* send command */
            send(s, buf, len, 0);

            /* wait for server to respond */
            recv(s, rbuf, sizeof(rbuf), 0);

            /* print server response */
            cout << rbuf;

            /* close socket */
            close(s);

            /* terminate the program */
            exit(1);
        }

        else if(strcmp(buf, "logout\n") == 0)
        {
            /* send command */
            send(s, buf, len, 0);

            /* wait for server response */
            recv(s, rbuf, sizeof(rbuf), 0);

            /* display message */
            cout << rbuf;
        }

        else if (first_word == "login")
        {
            send(s, buf, len, 0);
            recv(s, rbuf, sizeof(rbuf), 0);
            cout << rbuf; 
        }

	cout << "c: ";
    }

    close(s);
} 

