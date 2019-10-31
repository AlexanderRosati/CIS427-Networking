//Programmers: Alexander Rosati and Avian Calado
//OS: Red Hat Linux
//Compiler: g++
//Description: multithreaded server

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <map>

using namespace std;

#define PORT 8971 
#define MAX_LINE 256
#define MAX_NUM_MESSAGES 10

//Global Vars
fd_set master;
int listener; 
int fdmax;
int num_messages = 0;
int curr_message = 0;

// to store messages
string messages_of_the_day[MAX_NUM_MESSAGES];

// to store ip addresses and user ids
map<int, string> client_ips;
map<int, string> client_ids;

// create arrays for user names and passwords
string user_names[4];
string passwords[4];

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

// the child thread
void* ChildThread(void *newfd) 
{
    // vars
    char buf[MAX_LINE];
    int nbytes;
    int i, j;
    long childSocket = (long) newfd;
    int len = 0;
    bool msg_store = false;
    int user_pos = -1;

    while(1) 
    {
        // handle data from a client
        if ((nbytes = recv(childSocket, buf, sizeof(buf), 0)) <= 0) 
        {
            // got error or connection closed by client
            if (nbytes == 0) 
            {
                // connection closed
                cout << "multiThreadServer: socket " << childSocket <<" hung up" << endl;
            }
            
            else 
            {
                perror("recv");
            }

            // remove client's ip address and id
            client_ips.erase(childSocket);
            client_ids.erase(childSocket);

            // close thread's socket
            close(childSocket); 
            
            // remove socket from master set
            FD_CLR(childSocket, &master);

            // terminate the  thread
            pthread_exit(0);
        } 
       
        //if we receive something from the client associated with the thread 
        else
        {
            // print messaage that was sent to server
            cout << "msg from socket " << childSocket << ": "  << buf;

            // get first word and count number of spaces if necessary
            string first_word(buf);        
            int num_spaces = 0;

            if (has_space(first_word) && !msg_store)
            {
                for (int i = 0; i < first_word.size(); ++i)
                {
                    if (isspace(first_word.at(i)))
                    {
                        ++num_spaces;
                    }
                }

                // subtract out new line character
                --num_spaces; 

                // just get first word of input
                first_word = first_word.substr(0, first_word.find(' '));
            }
        
            // if it is a message to store
            if (msg_store)
            { 
                //vars
                string tempbuf(buf);
                ofstream write_message("messages.txt", ios_base::app);
                write_message << tempbuf;
                write_message.close();

                // Puts new MOTD at next empty cell
                messages_of_the_day[num_messages - 1] = tempbuf;

                // show what was stored in array and file
                cout << "Storing . . .\n\t" << messages_of_the_day[num_messages - 1] << "in file and internal data structure." << endl;

                // create message for client
                strcpy(buf, "200 OK\n");

                // get message len
                len = strlen(buf) + 1;

                // send response
                send(childSocket, buf, len, 0);

                // don't store next message
                msg_store = false;
            }

            //if message is a msgget
            else if (strcmp(buf, "msgget\n") == 0)
            {
                // create string to return
                string message_to_client = "200 OK\n";
                message_to_client += messages_of_the_day[curr_message];

                // copy message to client into buffer
                strcpy(buf, message_to_client.c_str());
            
                // get length of message
                len = strlen(buf) + 1;
 
                // move on to next message
                if (curr_message < num_messages - 1)
                {
                    ++curr_message;
                }

                else
                {
                    curr_message = 0;
                }

                // send to thread's client
                send(childSocket, buf, len, 0);     
            }

            // if we get the quit command
            else if (strcmp(buf, "quit\n") == 0)
            {
                // copy ok message to buf
                strcpy(buf, "200 OK\n");
                
                // get length of message
                len = strlen(buf) + 1;

                //send message to client
                send(childSocket, buf, len, 0);

                // close socket to thread's client
                close(childSocket);

                // say which client left
                cout << "multiThreadServer: socket " << childSocket <<" hung up" << endl;
               
                // remove client socket from set
                FD_CLR(childSocket, &master);

                // remove client's ip address and id
                client_ips.erase(childSocket);
                client_ids.erase(childSocket);

                // terminate thread
                pthread_exit(0);
            }

            // if message is message store
            else if (strcmp(buf, "msgstore\n") == 0)
            {
                // if user is NOT Logged in
		if (user_pos == -1) 
                {
                    // create error message
                    strcpy(buf, "401 You are not currently logged in, login first.\n");

                    // get message len 
                    len = strlen(buf) + 1;

                    // send message to client 
                    send(childSocket, buf, len, 0);
                }

                // if array is full
                else if (num_messages == MAX_NUM_MESSAGES)
                {
                    // create error message
                    strcpy(buf, "403 Cannot hold any more messages.\n");

                    // get message len
                    len = strlen(buf) + 1;

                    //send message to client
                    send(childSocket, buf, len, 0);
                }
						
                // if user is authorized
                else 
                {
                    // increase number of messages
                    ++num_messages;

                    // create messsage
                    strcpy(buf, "200 OK\n");

                    // get message len
                    len = strlen(buf) + 1;

                    // send message
                    send(childSocket, buf, len, 0);

                    // store what ever is send to server next
                    msg_store = true;

                    continue;											
                }		    
            }

            // if message is login
            else if (first_word == "login" && num_spaces == 2)
            {
                //vars
                string converted_buf(buf);
                string user_name;
                string password;

                int pos_first_space = 0;
                int pos_second_space = 0;
                bool user_name_found = false;
                bool pass_word_found = false;

                // test to see if user is already logged in 
                if (user_pos != -1)
                {
                    // create message for client 
                    strcpy(buf, "411 Already logged in logout first\n");

                    // get message len 
                    len = strlen(buf) + 1;

                    // send message to client 
                    send(childSocket, buf, len, 0);

                    continue;
                }

                // find spaces 
                pos_first_space = converted_buf.find(' ');
                pos_second_space = converted_buf.find(' ', pos_first_space + 1);       

                // get user name and password
                user_name = converted_buf.substr(
                                                  pos_first_space + 1,
                                                  pos_second_space - pos_first_space - 1);

                password = converted_buf.substr(
                                                 pos_second_space + 1,
                                                 converted_buf.size() - pos_second_space - 2);

                // see if user name is valid
                for (int i = 0; i < 4; ++i)
                {
                    if (user_names[i] == user_name)
                    {
                        user_name_found = true;
                        user_pos = i;
                        break;
                    }
                }

                // if user name is not valid 
                if (!user_name_found)
                {
                    // create message 
                    strcpy(buf, "410 Wrong UserID or Password\n");

                    // get message len 
                    len = strlen(buf) + 1;

                    // send message to client 
                    send(childSocket, buf, len, 0);

                    continue;
                 }

                 // see if given password is correct
                 if (passwords[user_pos] == password)
                 {
                     // store id of client
                     client_ids[childSocket] = user_name;

                     // create message 
                     strcpy(buf, "200 OK\n");

                     // get message len 
                     len = strlen(buf) + 1;

                     // send message to client 
                     send(childSocket, buf, len, 0);
                 }

                 // wrong password 
                 else
                 {
                     // create message 
                     strcpy(buf, "410 Wrong UserID or Password\n");

                     // get message len 
                     len = strlen(buf) + 1;

                     // send message to client 
                     send(childSocket, buf, len, 0);

                     // no one is logged in *
                     user_pos = -1;
                }
            }

            // if message is logout
            else if (strcmp(buf, "logout\n") == 0)
            {
                // if user is not logged in 
                if (user_pos == -1)
                {
                    // create message 
                    strcpy(buf, "412 Not logged in\n");  

                    // get message len 
                    len = strlen(buf) + 1; 

                    // send message to client 
                    send(childSocket, buf, len, 0);
                } 

                else
                {
                    // mark client as anonymous
                    client_ids[childSocket] = "anonymous";

                    // create message 
                    strcpy(buf, "200 OK\n");  

                    // get message len 
                    len = strlen(buf) + 1; 

                    // send message to client 
                    send(childSocket, buf, len, 0);

                    // set to -1 b/c user is no longer logged in 
                    user_pos = -1;
                } 
            }

            // if message is who
            else if (strcmp(buf, "who\n") == 0)
            {
                
            }

            // if message is shutdown
            else if (strcmp(buf, "shutdown\n") == 0)
            {
                // if the user is not the root
                if (user_pos != 0)
                {
                    // create message
                    strcpy(buf, "402 User not allowed to execute this command.\n");

                    // get length of message
                    len = strlen(buf) + 1;

                     // send message to thread's client
                    send(childSocket, buf, len, 0);
                }

                // if the user is the root
                else
                {    
                    // close listener
                    close(listener);

                    // remove listener from master set
                    FD_CLR(listener, &master); 
                    
                    // remove childSocket from master set
                    FD_CLR(childSocket, &master);

                    // message to send to all clients
                    strcpy(buf, "210 the server is about to shutdown ......\n");

                    // get length of message
                    len = strlen(buf) + 1;

                    // iterate through all clients except one that sent shutdown
                    for (int i = 0; i < fdmax + 1; ++i)
                    {
                        if (FD_ISSET(i, &master))
                        {
                            send(i, buf, len, 0);
                        }
                    }

                    // send 200 OK to client that sent shutdown
                    strcpy(buf, "200 OK");
                    len = strlen(buf) + 1;
                    send(childSocket, buf, len, 0);

                    // put thread to sleep for five ms
                    usleep(5 * 1000);

                    // send last message to client that sent shutdown
                    strcpy(buf, "210 the server is about to shutdown ......\n");
                    len = strlen(buf) + 1;
                    send(childSocket, buf, len, 0);
                   
                    // close childSocket
                    close(childSocket);

                    // show that client that sent shutdown disconnected
                    cout << "multiThreadServer: socket " << childSocket <<" hung up" << endl;
 
                    // put thread to sleep for five ms
                    usleep(5 * 1000);

                    // terminate all remanning threads
                    exit(0);
                }
            }

            // if message is send
            else if (first_word == "send" && num_spaces == 1)
            {
                
            }

            // if we get invalid input
            else
            {
                // put message in buf
                strcpy(buf, "300 message format error\n");
                
                // get length of message
                len = strlen(buf) + 1;

                // send message to thread's client
                send(childSocket, buf, len, 0); 
            }
        }
    }
}

int main(void)
{
    //vars
    struct sockaddr_in myaddr;     
    struct sockaddr_in remoteaddr; 
    int newfd;        
    int yes=1;        
    socklen_t addrlen;        
    pthread_t cThread;

    // read data in file into internal data structure
    fstream f_messages;
    f_messages.open("messages.txt");

    // make sure opening file was successful 
    if (!f_messages.is_open())
    {
        cout << "Failed to open messages.txt" << endl;
        exit(1);
    }

    // after while loop, num_messages will equal the number of messages and all of the messages will by in the array
    while(f_messages.peek() != EOF)
    {
        getline(f_messages, messages_of_the_day[num_messages]);
        ++num_messages;
    }
 
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

    // initialize user names and passwords
    user_names[0] = "root";
    user_names[1] = "john";
    user_names[2] = "david";
    user_names[3] = "mary";

    passwords[0] = "root01";
    passwords[1] = "john01";
    passwords[2] = "david01";
    passwords[3] = "mary01";

    // clear the master set
    FD_ZERO(&master);    

    // get the listener
    if ((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
    {
        perror("socket");
        exit(1);
    }

    // lose the pesky "address already in use" error message
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) 
    {
        perror("setsockopt");
        exit(1);
    }

    // bind
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = INADDR_ANY;
    myaddr.sin_port = htons(PORT);
    memset(&(myaddr.sin_zero), '\0', 8);

    if (bind(listener, (struct sockaddr *)&myaddr, sizeof(myaddr)) == -1) 
    {
        perror("bind");
        exit(1);
    }

    // listen
    if (listen(listener, 10) == -1) 
    {
        perror("listen");
        exit(1);
    }

    // add the listener to the master set
    FD_SET(listener, &master);

    // keep track of the biggest file descriptor
    fdmax = listener; 

    addrlen = sizeof(remoteaddr);

    // main loop
    for(;;) 
    {
        // handle new connections
        if ((newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen)) == -1) 
        {
            perror("accept");
	    exit(1);
        }
       
        // if someone connects 
        else
        {
            // add to master set
            FD_SET(newfd, &master); 

            cout << "multiThreadServer: new connection from "
                 << inet_ntoa(remoteaddr.sin_addr)
                 << " socket " << newfd << endl;
            
            // keep track of the maximum
            if (newfd > fdmax) 
            {    
                fdmax = newfd;
            }

            // store user ip and id
            client_ips[newfd] = inet_ntoa(remoteaddr.sin_addr);
            client_ids[newfd] = "anonymous";

	    if (pthread_create(&cThread, NULL, ChildThread, (void *) newfd) < 0) 
            {
                perror("pthread_create");
                exit(1);
            }
        }
    }
    
    return 0;
}
