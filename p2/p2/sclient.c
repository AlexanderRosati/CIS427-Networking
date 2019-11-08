//Programmers: Alexander Rosati and Avian Calado
//OS: Red Hat Linux
//Compiler: g++
//Description: client

#include <iostream>
#include <stdio.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <cstring>
#include <cstdlib>

using namespace std;

#define SERVER_PORT 8971
#define MAX_LINE 256
#define STDIN 0

//Description: lower cases all characters in buf
void to_lower_buf(char* buf, int num_chars)
{
    for (int i = 0; i < num_chars; ++i)
    {
        buf[i] = tolower(buf[i]);
    }
}

int main(int argc, char * argv[]) {

    fd_set master;   
    fd_set read_fds; 
    int fdmax;       

    struct sockaddr_in sin;
    char buf[MAX_LINE];
    int len;
    int s;

    bool quitting = false;
    bool msg_store = false;
    bool skip_quit = false;

    // clear the master and temp sets
    FD_ZERO(&master);    
    FD_ZERO(&read_fds);

    // make sure right number of arguments
    if (argc != 2)
    {
        cout << "usage: client <Server IP>" << endl;
        exit(1);
    }

    // active open 
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    {
		perror("select client: socket");
		exit(1);
    }

    // build address data structure 
    bzero((char *)&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr  = inet_addr(argv[1]);
    sin.sin_port = htons (SERVER_PORT);

    if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) 
    {
		perror("select client: connect");
		close(s);
		exit(1);
    }

    // add the STDIN to the master set
    FD_SET(STDIN, &master);

    // add the listener to the master set
    FD_SET(s, &master);

    // keep track of the biggest file descriptor
    fdmax = s; 

    // display welcome message
    cout << "You have entered the YAMOTD program. Please enter a command." << endl;

    // display cursor
    cout << "c: ";

    //manurally flush buffer
    cout.flush();

    // main loop; get and send lines of text 
    while (1) 
    {
        // copy master
        read_fds = master; 

        // wait for server to send something or user to input something
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) 
        {
            perror("select");
            exit(1);
        }

        // if user input is ready 
        if (FD_ISSET(STDIN, &read_fds)) 
        {
            // handle the user input
            if (fgets(buf, sizeof(buf), stdin))
            {
                buf[MAX_LINE -1] = '\0';
                len = strlen(buf) + 1;
                
                //lower case all characters in buf
                if (!msg_store)
                {
                    to_lower_buf(buf, strlen(buf));
                }

                else
                {
                    msg_store = false;
                    skip_quit = true;
                }

                //test to see if quitting
                if ((strcmp(buf, "quit\n") == 0) && !skip_quit)
                {
                    quitting = true;
                } 

                else
                {
                    skip_quit = false;
                }

                // if message is msgstore
                if (strcmp(buf, "msgstore\n") == 0)
                {
                    msg_store = true;
                }

				

                //send user's message to the server
                send(s, buf, len, 0);
            } 

            else 
            {
                break;
            }
        }
        
        // if there is a message from the server
        if (FD_ISSET(s, &read_fds)) 
        {
            // put the servers message in the buffer
            recv(s, buf, sizeof(buf), 0);
            
            // get code of message
            string message_code(buf);
            message_code = message_code.substr(0, 3);

            // if client is quitting
            if (quitting)
            {
                // fall out of while loop
                break;
            } 
            
            // shuttind down                
            if (message_code == "210")
            {
                cout << "\ns: " << buf;

                // fall out of while loop
                break;
            }
            
            // display what the server sent
            cout << "s: " << buf;
            
            // if we are shutting down then don't print cursor
            if (strcmp(buf, "200 OK") != 0)
            { 
                // display cursor
                cout << "c: ";
            }

            // manually flush buffer
            cout.flush();
        }    
    }

    close(s);
	return 0;
}
