#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <string>
#include <arpa/inet.h>
#include <signal.h>

/////////////////////////////
#define BUF 4069
#define PORT 6543
/////////////////////////////
void *clientCommunication(void *data) 
{
    char buffer[BUF];
    int abortRequested = 0;
    //has to be pointer so new socket address can be given to clientCommunictaion as parameter//
    int *currentSocket = (int *)data;

    ////////////////////////
    //SEND WELCOME MESSAGE//
    strcpy(buffer, "Welcome to the Server!\nPlease enter your commands...\n");
    if(send(*currentSocket, buffer, strlen(buffer), 0) == -1) {
        std::cerr << "Sending welcoming message failed" << std::endl;
        return NULL;
    }

    do
    {
        ///////////
        //RECEIVE//
        int size = recv(*currentSocket, buffer, BUF -1, 0);
        if(size == -1) {
            if(abortRequested) {
                std::cerr << "Receiving Error after aborted" << std::endl;
            }
            else{
                std::cerr << "Received Error" << std::endl;
            }
            break;
        }
        if(size == 0) {
            std::cerr << "Client closed remote socket" << std::endl;
            break;
        }

        buffer[size] = '\0';
        std::cout << "Message received: " << buffer << std::endl;
        if(send(*currentSocket, "OK\n", 3, 0) == -1) {
            std::cerr << "Sending an Answer failed" << std::endl;
            return NULL;
        }

    } while (strcmp(buffer, "quit") != 0 && !abortRequested);
    
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
    int abortRequested = 0;
    int newSocket = -1;
    int createSocket = -1;
    if(sig == SIGINT) {
        //ignore error
        std::cerr << "abort Requested... " << std::endl;
        abortRequested = 1;

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


int main(int argc, char argv[]) 
{
    socklen_t  addrlen;
    struct sockaddr_in address, clientAddress;
    //why do i need it???? Cause it is just a reused value?
    int reuseValue = 1;
    int abortRequested = 0;


    //////////////////
    //SIGNAL HANDLER//
    if(signal(SIGINT, signalHandler) == SIG_ERR) {
        std::cerr << "Signal can not be registered" << std::endl;
        return -7;
    }

    ///////////////////
    //CREATE A SOCKET//
    int createSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(createSocket == -1) {
        std::cerr << "Can't create a socket" << std::endl;
        return -1;
    }

    //////////////////////
    //SET SOCKET OPTIONS//
    //for multiple clients
    if(setsockopt(createSocket, SOL_SOCKET, SO_REUSEADDR, &reuseValue,sizeof(reuseValue)) == -1) {//i have a feeling this is wrong
        std::cerr << "Can't manipulate socket options??? - reuseAddr" << std::endl;
        return -5;
    }
    if(setsockopt(createSocket, SOL_SOCKET, SO_REUSEPORT, &reuseValue,sizeof(reuseValue)) == -1) {
        std::cerr << "Can't manipulate socket options??? - reusePort" << std::endl;
        return -6;
    }

    ////////////////
    //INIT ADDRESS//
    memset(&address ,0 ,sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT); //htons(host to network shorts) flips the bits and converts them from whole numbers to numbers networks can understand
    

    /////////////////////////////////////////
    //ASSIGN AN ADDRESS WITH PORT TO SOCKET//
    if(bind(createSocket, (struct sockaddr*)&address, sizeof(address)) == -1) {
        std::cerr << "Can't bind to a socket!" << std::endl;
        return -2;
    }

    /////////////////////////////////
    //ALLOW CONNECTION ESTABLISHING//
    //mark socket for listening in
    if(listen(createSocket, 6) == -1) {
        std::cerr << "Can't listen!" << std::endl;
        return -3;
    }

    while(!abortRequested) {
        std::cout << "Waiting for connections..." << std::endl;

        ///////////////////////////
        //ACCEPT CONNECTION SETUP//
        addrlen = sizeof(struct sockaddr_in);
        int newSocket = accept(createSocket, (struct sockaddr*)&clientAddress, &addrlen);
        if(newSocket == -1) {
            if(abortRequested) {
                std::cerr << "Accept error after it was aborted" << std::endl;
            }
            else {
                std::cerr << "Accept error" << std::endl;
            }
            break;
        }

        ////////////////
        //START CLIENT//
        std::cerr << "Client connected from " << inet_ntoa(clientAddress.sin_addr) << ":" << ntohs(clientAddress.sin_port) << "..." << std::endl;
        clientCommunication(&newSocket);
        newSocket = -1; //why??? -1 is an error no?
    }

    //frees the descriptor//
    if(createSocket != -1) {
        if(shutdown(createSocket, SHUT_RDWR) == -1) {
            std::cerr << "Shutdown createSocket" << std::endl;
        }
        if(close(createSocket) == -1) {
            std::cerr << "Close createSocket" << std::endl;
        }

        createSocket = -1;
    }

    return 0;

}