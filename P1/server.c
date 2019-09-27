/*
 * server.c
 */

#include <stdio.h>
#include <iostream>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <netdb.h>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <string>

using namespace std;

#define SERVER_PORT 2403
#define MAX_PENDING 5
#define MAX_LINE 256

//Description: returns true if sting has a space
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

int main(int argc, char **argv) 
{

    struct sockaddr_in sin;
    socklen_t addrlen;
    char buf[MAX_LINE];
    int len;
    int s;
    int new_s;
    string messages_of_the_day[100];
    int num_messages = 0;
    int curr_message = 0;
    int message_len = 0;
    char msg[MAX_LINE];
    string user_names[4];
    string passwords[4];

    /* tells you which user is logged in
    * -1 means no one is logged in
    * 0 means root is logged in */ 
    int user_pos = -1;

    /* initialize user names and passwords */
    user_names[0] = "root";
    user_names[1] = "john";
    user_names[2] = "david";
    user_names[3] = "mary";

    passwords[0] = "root01";
    passwords[1] = "john01";
    passwords[2] = "david01";
    passwords[3] = "mary01";

    /* display user names and passwords on server */
    cout << endl;

    for (int i = 0; i < 4; ++i)
    {
        cout << user_names[i] << " has password " << passwords[i] << endl;
    }

    /* read data in file into internal data structure */
    ifstream f_messages;
    f_messages.open("messages.txt");

    /* make sure opening file was successful */
    if (!f_messages.is_open())
    {
        cout << "Failed to open messages.txt" << endl;
        exit(1);
    }

    /* after while loop, num_messages will equal the number of messages and all of the messages will by in the array */
    while(!f_messages.eof())
    {
        getline(f_messages, messages_of_the_day[num_messages]);
        ++num_messages;
    }

    /* num_messages was incremented one too many times */
    num_messages -= 1;

    cout << endl;

    for (int i = 0; i < num_messages; ++i)
    {
        cout << "Loading message: " << messages_of_the_day[i] << endl;

        /* append new line character to message */
        messages_of_the_day[i] += '\n';
    }

    cout << "Num messages: " << num_messages << endl;
    cout << endl;

    f_messages.close();

    /* build address data structure */
    bzero((char *)&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons (SERVER_PORT);

    /* setup passive open */
    if (( s = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(1);
    }

    if ((bind(s, (struct sockaddr *) &sin, sizeof(sin))) < 0) {
		perror("bind");
		exit(1);
    }

    listen (s, MAX_PENDING);

    addrlen = sizeof(sin);
    cout << "The server is up, waiting for connection" << endl;

    /* wait for connection, then receive and print text */
    while (1) 
    {
		if ((new_s = accept(s, (struct sockaddr *)&sin, &addrlen)) < 0) 
                {
			perror("accept");
			exit(1);
		}

		cout << "new connection from " << inet_ntoa(sin.sin_addr) << endl;
	
		while (len = recv(new_s, buf, sizeof(buf), 0)) 
                {
                    /* print msg received */
                    cout << "msg: " << buf; 

                    /* determine if user is trying to login */
                    string first_word(buf);
                    
                    if (has_space(first_word))
                    {
                        /* just get first word of input */
                        first_word = first_word.substr(0, first_word.find(' '));
                    }
                    
                    if (strcmp(buf, "msgget\n") == 0)
                    {
                        /* make a temp string */ 
                        string temp = "s: 200 OK\n";
                        temp += messages_of_the_day[curr_message];

                        /* turn str to c_str and calculate length */
                        strcpy(msg,  temp.c_str());
                        message_len = strlen(msg) + 1;

                        /* move on to next meessage */
                        if (curr_message < num_messages - 1)
                        {
                            ++curr_message;
                        }

                        else
                        {
                            curr_message = 0;
                        }

                        send(new_s, msg, message_len, 0);
                    }  

                    else if (strcmp(buf, "quit\n") == 0)
                    {
                        /* create message for client */
                        strcpy(msg, "s: 200 OK\n");

                        /* get message len */
                        message_len = strlen(msg) + 1; 

                        /* send response */
                        send(new_s, msg, message_len, 0);

                        /* set to -1 b/c the user has logged out */
                        user_pos = -1;
		    }

                    else if (strcmp(buf, "logout\n") == 0)
                    {
                       /* if user is not logged in */
                       if (user_pos == -1)
                       {
                           /* create message */
                           strcpy(msg, "s: 412 Not logged in\n");  

                           /* get message len */
                           message_len = strlen(msg) + 1; 

                           /* send message to client */
                           send(new_s, msg, message_len, 0);
                       } 

                       else
                       {
                           /* create message */
                           strcpy(msg, "s: 200 OK\n");  

                           /* get message len */
                           message_len = strlen(msg) + 1; 

                           /* send message to client */
                           send(new_s, msg, message_len, 0);

                           /* set to -1 b/c user is no longer logged in */
                           user_pos = -1;
                       }

                    }

                    else if (first_word == "login")
                    {
                       /* make sure there are two spaces */
                       string converted_buf(buf);
                       string user_name;
                       string password;

                       int num_spaces = 0;
                       int pos_first_space = 0;
                       int pos_second_space = 0;
                       
                       bool user_name_found = false;
                       bool pass_word_found = false;

                       /* test to see if user is already logged in */
                       if (user_pos != -1)
                       {
                           /* create message for client */
                           strcpy(msg, "411 Already logged in logout first\n");

                           /* get message len */
                           message_len = strlen(msg) + 1;

                           /* send message to client */
                           send(new_s, msg, message_len, 0);

                           continue;
                       }

                       for (int i = 0; i < converted_buf.size(); ++i)
                       {
                           if (isspace(converted_buf.at(i)))
                           {
                               ++num_spaces;
                           }
                       }

                       /* subtract out new line character */
                       --num_spaces;

                       /* if message is not correctly formated */
                       if (num_spaces != 2)
                       {
                           /* message length for client */
                           strcpy(msg, "s: 300 message format error\n");

                           /* get message len */
                           message_len = strlen(msg) + 1;

                           /* send message to client */
                           send(new_s, msg, message_len, 0);

                           continue;
                        }

                       /* find space */
                       pos_first_space = converted_buf.find(' ');
                       pos_second_space = converted_buf.find(' ', pos_first_space + 1); 
                      
                       /* get user name and password */ 
                       user_name = converted_buf.substr(
                                                        pos_first_space + 1,
                                                        pos_second_space - pos_first_space - 1);
                       password = converted_buf.substr(
                                                       pos_second_space + 1,
                                                       converted_buf.size() - pos_second_space - 2);

                      /* see if user name is valid */
                      for (int i = 0; i < 4; ++i)
                      {
                          if (user_names[i] == user_name)
                          {
                              user_name_found = true;
                              user_pos = i;
                              break;
                          }
                      } 

                      /* if user name is not valid */
                      if (!user_name_found)
                      {
                         /* create message */
                         strcpy(msg, "s: 410 Wrong UserID or Password\n");

                         /* get message len */
                         message_len = strlen(msg) + 1;

                         /* send message to client */
                         send(new_s, msg, message_len, 0);

                         continue;
                      }

                      /* see if given password is correct */
                      if (passwords[user_pos] == password)
                      {
                         /* create message */
                         strcpy(msg, "s: 200 OK\n");

                         /* get message len */
                         message_len = strlen(msg) + 1;

                         /* send message to client */
                         send(new_s, msg, message_len, 0);
                      }

                      /* wrong password */
                      else
                      {
                         /* create message */
                         strcpy(msg, "s: 410 Wrong UserID or Password\n");

                         /* get message len */
                         message_len = strlen(msg) + 1;

                         /* send message to client */
                         send(new_s, msg, message_len, 0);

                         /* no one is logged in */ 
                         user_pos = -1;
                      }

                    }
                }
                
                /* close client socket */
                close(new_s);
               
                /* print message to terminal */
                cout << inet_ntoa(sin.sin_addr) << " is no longer connected with the server\n";
    } 
} 
