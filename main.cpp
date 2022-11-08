#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <string>
#include <arpa/inet.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <dirent.h>
#include <vector>
#include <filesystem>
#include <chrono>
#include <ctime>
#include <sys/stat.h>

namespace fs = std::filesystem;

/////////////////////////////
#define BUF 4096
int PORT;
/////////////////////////////
int abortRequested = 0;
int createSocket = -1;
int newSocket = -1;
//////////////////////////////

void *clientCommunication(void *data, char folder[30]) 
{
    char buffer[BUF];
    int size;
    int *currentSocket = (int *)data;


///////////////////////////////////////////////////////////////////////////
    //SEND WELCOME MESSAGE//
    strcpy(buffer, "Welcome to the Server!\nPlease enter your commands...\n");
    if(send(*currentSocket, buffer, strlen(buffer), 0) == -1) {
        std::cerr << "Sending welcoming message failed";
        return NULL;
    }

    do
    {
///////////////////////////////////////////////////////////////////////////////////////
        //RECEIVE//
        size = recv(*currentSocket, buffer, BUF -1, 0);
        if(size == -1) {
            if(abortRequested) {
                std::cerr << "Receiving Error after aborted";
            }
            else{
                std::cerr << "Received Error" << std::endl;
            }
            break;
        }
        if(size == 0) {
            std::cerr << "Client closed remote socket";
            break;
        }

        // remove ugly debug message, because of the sent newline of client
        if (buffer[size - 2] == '\r' && buffer[size - 1] == '\n')
        {
            size -= 2;
        }
        else if (buffer[size - 1] == '\n')
        {
            --size;
        }

        buffer[size] = '\0';
        std::cout << "Message received: " << buffer << std::endl;

        std::stringstream str(buffer);
        std::string line;
        std::vector<std::string> msg;

        while(std::getline(str, line, '\n')) {
            msg.push_back(line);
        }

////////////////////////
        //SEND//
        if(msg.at(0).compare("SEND") == 0) {
            //creates subfolder in directory with name of receiver//
            fs::create_directory(folder + msg.at(2));
            //makes the filename the time it was sent//
            auto now = std::chrono::system_clock::now();
            auto timeT = std::chrono::system_clock::to_time_t(now);
            char* time = std::ctime(&timeT);
            time[strlen(time) -1] = '\0';
            std::ofstream file(folder + msg.at(2) + "/" + time);
            file << msg.at(1) << std::endl << msg.at(3) << std::endl << msg.at(4);
            file.close();
        } 

////////////////////
        //LIST//
        if(msg.at(0).compare("LIST") == 0) {
            //i dont think path is right//
            if(msg.at(1).compare(fs::path(folder+ msg.at(1)))) {
                std::ifstream infile;
                infile.open(folder + msg.at(1));
                if(infile.is_open()) {
                    //doesnt go into while//
                    //cause getline if for input not output?//
                    while(std::getline(infile, line, '\n')) {
                        std::cout << line << std::endl;
                    }
                }
            }else {
                std::cout << "User was not found in the directory!" << std::endl;
            }
        }

        //empties the buffer of the client messages and writes an OK in it//
        memset(buffer, 0, BUF);
        strcat(buffer, "OK");

        if(send(*currentSocket, buffer, BUF, 0) == -1) {
            //just because send was successfully completed does not
            //mean message was received, so this looks if there are
            //locally-detected errors
            memset(buffer, 0, BUF);
            strcat(buffer, "ERR");
            send(*currentSocket, buffer, BUF, 0);
            std::cerr << "Sending local Error" << std::endl;
            break;
        }

    } while (strcmp(buffer, "QUIT") != 0 && !abortRequested);
    
    //closes/frees the descriptor if its not already//
    if(*currentSocket == -1) {
        if(shutdown(*currentSocket, SHUT_RDWR) == -1) {
            std::cerr << "Shutdown newSocket" << std::endl;
        }
        if(close(*currentSocket)) {
            std::cerr << "Closes newSocket" << std::endl;
        }

        *currentSocket = -1;
    }

    return NULL;
}


void signalHandler(int sig)
{
    int newSocket = -1;
    int createSocket = -1;
    if(sig == SIGINT) {
        //ignore error
        std::cerr << "abort Requested... " << std::endl;
        exit(0);

        if(newSocket != -1) {
            if(shutdown(newSocket, SHUT_RDWR) == -1) {
                std::cerr << "Shutdown newSocket" << std::endl;
            }
            if(close(newSocket) == -1) {
                std::cerr << "Close newSocket" << std::endl;
            }

            newSocket = -1;
        }

        if(createSocket != -1) {
            if(shutdown(createSocket, SHUT_RDWR) == -1) {
                std::cerr << "Shutdown createSocket" << std::endl;
            }
            if(close(createSocket) == -1) {
                std::cerr << "Close createSocket" << std::endl;
            }

            createSocket = -1;
        }
    }
    else{
        exit(sig);
    }
}
//////////////////////////////////


int main(int argc, char** argv) 
{
    socklen_t  addrlen;
    struct sockaddr_in address, clientAddress;
    int reuseValue = 1;
//////////////////////////////////////
//

    if(argc < 2) {
        std::cerr << "More arguments needed.";
    }
    else{
        //assign a PORT number//
        PORT = atoi(argv[1]);

        //checking if folder exists//
        DIR* folder = opendir(argv[2]);

        if(!folder) {
            //create directory//
            int exists = mkdir(argv[2], 0777);
            DIR* folder = opendir(argv[2]);
            if(!exists) {
                std::cout << "Directory created" << std::endl;
            } else {
                std::cerr << "Unable to create directory" << std::endl;
                exit(1);
            } 
        }
    }

//////////////////////////////////////////////////////////////////////////
    //SIGNAL HANDLER//
    if(signal(SIGINT, signalHandler) == SIG_ERR) {
        std::cerr << "Signal can not be registered";
        return -7;
    }

//////////////////////////////////////////////////////////////////////////
    //CREATE A SOCKET//
    int createSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(createSocket == -1) {
        std::cerr << "Can't create a socket";
        return -1;
    }

////////////////////////////////////////////////////////////////////////
    //SET SOCKET OPTIONS//
    if(setsockopt(createSocket, SOL_SOCKET, SO_REUSEADDR, &reuseValue, sizeof(reuseValue)) == -1) {//i have a feeling this is wrong
        std::cerr << "Set socket options - reuseAddr";
        return -5;
    }
    if(setsockopt(createSocket, SOL_SOCKET, SO_REUSEPORT, &reuseValue,sizeof(reuseValue)) == -1) {
        std::cerr << "Set socket options - reusePort";
        return -6;
    }

/////////////////////////////////////////////////////////////////////
    //INIT ADDRESS//
    memset(&address ,0 ,sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    //htons(host to network shorts) flips the bits and converts them from whole numbers
    //to numbers networks can understand
    address.sin_port = htons(PORT); 
    

////////////////////////////////////////////////////////////////////////////////
    //ASSIGN AN ADDRESS WITH PORT TO SOCKET//
    if(bind(createSocket, (struct sockaddr*)&address, sizeof(address)) == -1) {
        std::cerr << "Can't bind to a socket!" << std::endl;
        return -2;
    }

/////////////////////////////////////////////////////////////////////////////////
    //ALLOW CONNECTION ESTABLISHING//
    //mark socket for listening in
    if(listen(createSocket, 5) == -1) {
        std::cerr << "Can't listen!" << std::endl;
        return -3;
    }

    while(!abortRequested) {
        std::cout << "Waiting for connections..." << std::endl;

//////////////////////////////////////////////////////////////////////////////////////
        //ACCEPT CONNECTION SETUP//
        addrlen = sizeof(struct sockaddr_in);
        newSocket = accept(createSocket, (struct sockaddr*)&clientAddress, &addrlen);
        if(newSocket == -1) {
            if(abortRequested) {
                std::cerr << "Accept error after it was aborted" << std::endl;
            }
            else {
                std::cerr << "Accept error";
            }
            break;
        }

///////////////////////////////////////////////////////////////////////////////////////////////
        //START CLIENT//
        std::cout << "Client connected from " << inet_ntoa(clientAddress.sin_addr) << ":" << ntohs(clientAddress.sin_port) << "..." << std::endl;
        clientCommunication(&newSocket, argv[2]);//for directory
        newSocket = -1;
    }

    //frees the descriptor//
    if(createSocket != -1) {
        if(shutdown(createSocket, SHUT_RDWR) == -1) {
            std::cerr << "Shutdown createSocket";
        }
        if(close(createSocket) == -1) {
            std::cerr << "Close createSocket";
        }

        createSocket = -1;
    }

    return 0;

}