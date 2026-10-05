#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <string>

#include "../database/database.h"
#include "../command/command.h"

using namespace std;

class TCPServer {

private:

    int serverSocket;
    int port;

    Database db;
    CommandHandler handler;

    void handleClient(int clientSocket);

    bool sendAll(int clientSocket, const string& response);

public:

    TCPServer(int port);

    void start();
};

#endif