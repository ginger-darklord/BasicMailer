#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <string>
#include <arpa/inet.h>
#include <signal.h>

//////////////////////////
#define BUF 1024
#define PORT 6543
//////////////////////////

int main(int argc, char **argv)
{
    char buffer[BUF];
    struct sockaddr_in address;
    int isQuit = 0;
    int size;

    /////////////////
    //CREATE SOCKET//
    int createSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(createSocket == -1) {
        std::cerr << "Can't create a socket" << std::endl;
        return -1;
    }

    ////////////////
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

    /////////////////////
    //CREATE CONNECTION//
    if(connect(createSocket, (struct sockaddr*) &address, sizeof(address)) == -1) {
        std::cerr << "Connection Error - no Server available" << std::endl;
        return -2;
    }

    std::cout << "Connection with server " << inet_ntoa(address.sin_addr) << " established" << std::endl;

    ////////////////
    //RECEIVE DATA//
    size = recv(createSocket, buffer, BUF - 1, 0);
    if(size == -1) {
        std::cerr << "Could not receive data" << std::endl;
    }
    else if(size == 0) {
        std::cerr << "The Server close the Remote Socket" << std::endl;
    }
    else{
        buffer[size] = '\0';
        std::cout << buffer << std::endl;
    }

    do{
        std::cout << "Server Commands : SEND, LIST, READ, DEL, QUIT. Type them in lowercase" << std::endl;
        std::cout << ">> " << std::endl;
        if(fgets(buffer, BUF, stdin) != NULL) {
            int size = strlen(buffer);
            isQuit = strcmp(buffer, "quit") == 0;
            if(strcmp(buffer,"send") == 0) {
                /////////////
                //SEND DATA//
                if(send(createSocket, buffer, size, 0) == -1) {
                    //just because send was successfully completed does not
                    //mean message was received, so this looks if there are
                    //locally-detected errors
                    std::cerr << "Sending Error" << std::endl;
                    break;
                }
            }

            //////////////////////////////////////////////////////////////////////
         // RECEIVE FEEDBACK
         // consider: reconnect handling might be appropriate in somes cases
         //           How can we determine that the command sent was received 
         //           or not? 
         //           - Resend, might change state too often. 
         //           - Else a command might have been lost.
         //
         // solution 1: adding meta-data (unique command id) and check on the
         //             server if already processed.
         // solution 2: add an infrastructure component for messaging (broker)
         //

            size = recv(createSocket,buffer, BUF -1, 0);
            if(size == -1) {
                if(size == -1) {
                    std::cerr << "Message was not received - Receiving Error" << std::endl;
                    break;
                }
                else if(size == 0) {
                    std::cout << "Server closed the Remote Socket" << std::endl;
                }
                else{
                    buffer[size] = '\0';
                    std::cout << "<< " << buffer << std::endl;
                    if(strcmp("OK", buffer) != 0) {
                        //fprintf in c++?
                        std::cerr << "Server error occured, abort" << std::endl;
                        break;
                    }
                }
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
