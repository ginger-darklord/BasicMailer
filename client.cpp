#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <string>
#include <arpa/inet.h>
#include <signal.h>
#include <fstream>
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>

//////////////////////////
#define BUF 4096
#define PORT 6543
//////////////////////////

int main(int argc, char **argv)
{
    char buffer[BUF];
    struct sockaddr_in address;
    int isQuit;
    int size;

///////////////////////////////////////////////////////////////////////////////////
    //CREATE SOCKET//
    int createSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(createSocket == -1) {
        std::cerr << "Can't create a socket" << std::endl;
        return -1;
    }

////////////////////////////////////////////////////// ////////////////
    //INIT ADDRESS//
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);

    if(argc < 2) {
        inet_aton("127.0.0.1", &address.sin_addr);
    }
    else{
        inet_aton(argv[1], &address.sin_addr);
    }

////////////////////////////////////////////////////////////////////////////////////////
    //CREATE CONNECTION//
    if(connect(createSocket, (struct sockaddr*) &address, sizeof(address)) == -1) {
        std::cerr << "Connection Error - no Server available" << std::endl;
        return -2;
    }

    std::cout << "Connection with server " << inet_ntoa(address.sin_addr) << " established" << std::endl;

///////////////////////////////////////////////////////////////////////////////////////
    //RECEIVE DATA//
    size = recv(createSocket, buffer, BUF - 1, 0);
    if(size == -1) {
        std::cerr << "Could not receive data";
    }
    else if(size == 0) {
        std::cerr << "The Server closed remote Socket";
    }
    else{
        buffer[size] = '\0';
        std::cout << buffer << std::endl;
    }

    std::string command;
    do{
        memset(buffer, 0, BUF);
        std::cout << ">> ";
        std::cin >> command;

        
        isQuit = command.compare("QUIT") == 0;

/////////////////////////////////////////////////////////////////////////////////////
        //SEND DATA//
        char sender[8];
        char receiver[8];
        std::string subject;
        std::string message;
        std::string time;
        char user[8];

////////////////////////////
        //SEND//
        if(command.compare("SEND") == 0) {
            std::cout << "Sender: ";
            std::cin >> sender;

            std::cout << "Receiver: ";
            std::cin >> receiver;

            std::cout << "Subject: ";
            std::getline(std::cin >> std::ws, subject);

            std::cout << "Message: ";
            std::getline(std::cin >> std::ws, message);

            strcat(buffer,(command + "\n").c_str());
            strcat(buffer, sender);
            strcat(buffer, "\n");
            strcat(buffer, receiver);
            strcat(buffer, "\n");
            strcat(buffer, subject.c_str());
            strcat(buffer, "\n");
            strcat(buffer, message.c_str());
            strcat(buffer, ".\n");
        }

///////////////////////////////
        //LIST//
        if(command.compare("LIST") == 0) {
            std::cout << "User: ";
            std::cin >> user;

            strcat(buffer, (command + "\n").c_str());
            strcat(buffer, user);
        }

/////////////////////////////
        //READ//
        if(command.compare("READ") == 0) {
            std::cout << "User: ";
            std::cin >> user;
            std::cout << "The specific message: ";
            std::cin >> time;

            strcat(buffer, (command + "\n").c_str());
            strcat(buffer, user);
            strcat(buffer, time.c_str());
        }

/////////////////////////////
        //DEL//
        if(command.compare("DEL") == 0) {
            std::cout << "The message you want to remove: ";
            std::cin >> time;

            strcat(buffer, (command + "\n").c_str());
            strcat(buffer, time.c_str());
        }

//////////////////////////////
        //QUIT//
                    
        if(send(createSocket, buffer, size, 0) == -1) {
            //just because send was successfully completed does not
            //mean message was received, so this looks if there are
            //locally-detected errors
            std::cerr << "Sending Error" << std::endl;
            break;
        }

        size = recv(createSocket, buffer, BUF -1, 0);
        if(size == -1) {
            std::cerr << "Message was not received - Receiving Error";
            break;
        }
        else if(size == 0) {
            std::cout << "Server closed the Remote Socket" << std::endl;
            break;
        }
        else{
            int size = strlen(buffer);

            //removes new line signs from string at the end
            if (buffer[size - 2] == '\r' && buffer[size - 1] == '\n')
            {
                size -= 2;
                buffer[size] = 0;
            }
            else if (buffer[size - 1] == '\n')
            {
                --size;
                buffer[size] = 0;
            }

            buffer[size] = '\0';
            std::cout << "<< " << buffer << std::endl;
            if(strcmp("OK", buffer) != 0) {
                std::cerr << "Server error occured, abort";
                break;
            }
        }
    } while(!isQuit);

    /////////////////////////
    //CLOSES THE DESCRIPTOR//
    if(createSocket != -1) {
        if(shutdown(createSocket,SHUT_RDWR) == -1) {
            //invalid in case the server is gone already
            std::cerr << "Shutdown createSocket" << std::endl;
        }
        if(close(createSocket) == -1) {
            std::cerr << "Close createSocket" << std::endl;
        }
        createSocket = -1;
    }

    return 0;
}
