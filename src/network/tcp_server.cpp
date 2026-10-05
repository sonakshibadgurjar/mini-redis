#include "tcp_server.h"

#include "../protocol/resp.h"

#include <iostream>
#include <cstring>
#include <thread>
#include <cctype>
#include <sstream>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

using namespace std;


// ============================================================
// CONSTRUCTOR
// ============================================================

TCPServer::TCPServer(int port)
    : port(port), handler(db) {

    serverSocket = socket(
        AF_INET,
        SOCK_STREAM,
        0
    );

    if (serverSocket < 0) {

        cerr << "Socket creation failed!" << endl;

        exit(1);
    }


    int opt = 1;

    setsockopt(
        serverSocket,
        SOL_SOCKET,
        SO_REUSEADDR,
        &opt,
        sizeof(opt)
    );
}


// ============================================================
// SEND ALL
// ============================================================

bool TCPServer::sendAll(
    int clientSocket,
    const string& response
) {

    size_t totalSent = 0;

    while (totalSent < response.size()) {

        ssize_t sent = send(
            clientSocket,
            response.c_str() + totalSent,
            response.size() - totalSent,
            0
        );

        if (sent <= 0) {
            return false;
        }

        totalSent += sent;
    }

    return true;
}


// ============================================================
// PROCESS INLINE COMMAND
// ============================================================

static string processInlineCommand(
    CommandHandler& handler,
    const string& request
) {

    try {

        return handler.execute(request);

    }
    catch (...) {

        return RESP::error(
            "ERR command execution failed"
        );
    }
}


// ============================================================
// CLIENT HANDLER
// ============================================================

void TCPServer::handleClient(int clientSocket) {

    cout << "Client connected!" << endl;


    char buffer[4096];

    string requestBuffer;


    while (true) {

        ssize_t bytesReceived = recv(
            clientSocket,
            buffer,
            sizeof(buffer),
            0
        );


        // ----------------------------------------------------
        // CLIENT DISCONNECTED
        // ----------------------------------------------------

        if (bytesReceived == 0) {

            cout << "Client disconnected."
                 << endl;

            break;
        }


        // ----------------------------------------------------
        // RECEIVE ERROR
        // ----------------------------------------------------

        if (bytesReceived < 0) {

            cerr << "Receive error."
                 << endl;

            break;
        }


        // ----------------------------------------------------
        // ADD DATA TO BUFFER
        // ----------------------------------------------------

        requestBuffer.append(
            buffer,
            bytesReceived
        );


        // ====================================================
        // PROCESS BUFFER
        // ====================================================

        while (!requestBuffer.empty()) {


            // =================================================
            // RESP REQUEST
            // =================================================

            if (requestBuffer[0] == '*') {

                /*
                 * RESP array:
                 *
                 *2\r\n
                 *$3\r\nGET\r\n
                 *$4\r\nname\r\n
                 */


                // Find first CRLF
                size_t firstCRLF =
                    requestBuffer.find("\r\n");


                if (firstCRLF == string::npos) {

                    // Request is incomplete.
                    break;
                }


                // Read number of elements
                int argumentCount;

                try {

                    argumentCount =
                        stoi(
                            requestBuffer.substr(
                                1,
                                firstCRLF - 1
                            )
                        );

                }
                catch (...) {

                    string response =
                        RESP::error(
                            "ERR invalid RESP request"
                        );

                    sendAll(
                        clientSocket,
                        response
                    );

                    requestBuffer.clear();

                    continue;
                }


                if (argumentCount <= 0) {

                    string response =
                        RESP::error(
                            "ERR invalid RESP array"
                        );

                    sendAll(
                        clientSocket,
                        response
                    );

                    requestBuffer.erase(
                        0,
                        firstCRLF + 2
                    );

                    continue;
                }


                // Position after *N\r\n
                size_t position =
                    firstCRLF + 2;


                vector<string> arguments;

                bool incomplete = false;

                bool invalid = false;


                // ------------------------------------------------
                // Read each bulk string
                // ------------------------------------------------

                for (
                    int i = 0;
                    i < argumentCount;
                    i++
                ) {

                    // Need at least '$'
                    if (
                        position >=
                        requestBuffer.size()
                    ) {

                        incomplete = true;
                        break;
                    }


                    // Every argument must start with $
                    if (
                        requestBuffer[position] != '$'
                    ) {

                        invalid = true;
                        break;
                    }


                    // Find length CRLF
                    size_t lengthCRLF =
                        requestBuffer.find(
                            "\r\n",
                            position
                        );


                    if (
                        lengthCRLF ==
                        string::npos
                    ) {

                        incomplete = true;
                        break;
                    }


                    int length;

                    try {

                        length =
                            stoi(
                                requestBuffer.substr(
                                    position + 1,
                                    lengthCRLF -
                                    position - 1
                                )
                            );

                    }
                    catch (...) {

                        invalid = true;
                        break;
                    }


                    if (length < 0) {

                        invalid = true;
                        break;
                    }


                    // Move to actual data
                    position =
                        lengthCRLF + 2;


                    // Check whether complete data arrived
                    if (
                        position +
                        static_cast<size_t>(length) +
                        2 >
                        requestBuffer.size()
                    ) {

                        incomplete = true;
                        break;
                    }


                    // Extract value
                    string value =
                        requestBuffer.substr(
                            position,
                            length
                        );


                    arguments.push_back(value);


                    // Move after value
                    position += length;


                    // Must have CRLF
                    if (
                        requestBuffer[position] != '\r' ||
                        requestBuffer[position + 1] != '\n'
                    ) {

                        invalid = true;
                        break;
                    }


                    position += 2;
                }


                // ------------------------------------------------
                // Incomplete RESP request
                // ------------------------------------------------

                if (incomplete) {

                    break;
                }


                // ------------------------------------------------
                // Invalid RESP request
                // ------------------------------------------------

                if (invalid) {

                    string response =
                        RESP::error(
                            "ERR invalid RESP request"
                        );

                    sendAll(
                        clientSocket,
                        response
                    );


                    /*
                     * Remove the malformed request.
                     * This prevents the server from getting
                     * stuck on the same bad bytes.
                     */

                    size_t newline =
                        requestBuffer.find(
                            "\r\n"
                        );


                    if (
                        newline !=
                        string::npos
                    ) {

                        requestBuffer.erase(
                            0,
                            newline + 2
                        );

                    } else {

                        requestBuffer.clear();
                    }


                    continue;
                }


                // ------------------------------------------------
                // Build command string
                // ------------------------------------------------

                string command;


                for (
                    size_t i = 0;
                    i < arguments.size();
                    i++
                ) {

                    if (i > 0) {
                        command += " ";
                    }

                    command += arguments[i];
                }


                cout
                    << "Received RESP command: "
                    << command
                    << endl;


                // ------------------------------------------------
                // Execute
                // ------------------------------------------------

                string response =
                    processInlineCommand(
                        handler,
                        command
                    );


                // ------------------------------------------------
                // Send response
                // ------------------------------------------------

                if (
                    !sendAll(
                        clientSocket,
                        response
                    )
                ) {

                    cerr << "Send failed."
                         << endl;

                    close(clientSocket);

                    return;
                }


                // ------------------------------------------------
                // Check QUIT / EXIT
                // ------------------------------------------------

                string commandName =
                    arguments[0];


                for (char& c : commandName) {

                    c = static_cast<char>(
                        toupper(
                            static_cast<unsigned char>(c)
                        )
                    );
                }


                // Remove complete RESP request
                requestBuffer.erase(
                    0,
                    position
                );


                if (
                    commandName == "QUIT" ||
                    commandName == "EXIT"
                ) {

                    cout
                        << "Client requested "
                        << commandName
                        << "."
                        << endl;

                    close(clientSocket);

                    return;
                }


                continue;
            }


            // =================================================
            // INLINE COMMAND
            // =================================================

            /*
             * Example:
             *
             * PING\n
             * SET name Sonakshi\n
             * GET name\n
             */

            size_t newlinePosition =
                requestBuffer.find('\n');


            // Complete line has not arrived yet
            if (
                newlinePosition ==
                string::npos
            ) {

                break;
            }


            // Extract line
            string request =
                requestBuffer.substr(
                    0,
                    newlinePosition
                );


            // Remove processed data
            requestBuffer.erase(
                0,
                newlinePosition + 1
            );


            // Remove \r
            if (
                !request.empty() &&
                request.back() == '\r'
            ) {

                request.pop_back();
            }


            // Ignore empty lines
            if (request.empty()) {
                continue;
            }


            cout
                << "Received: "
                << request
                << endl;


            // Execute command
            string response =
                processInlineCommand(
                    handler,
                    request
                );


            // Send response
            if (
                !sendAll(
                    clientSocket,
                    response
                )
            ) {

                cerr << "Send failed."
                     << endl;

                close(clientSocket);

                return;
            }


            // Get command name
            string commandName;

            size_t spacePosition =
                request.find(' ');


            if (
                spacePosition ==
                string::npos
            ) {

                commandName =
                    request;

            } else {

                commandName =
                    request.substr(
                        0,
                        spacePosition
                    );
            }


            // Convert to uppercase
            for (char& c : commandName) {

                c = static_cast<char>(
                    toupper(
                        static_cast<unsigned char>(c)
                    )
                );
            }


            // QUIT / EXIT
            if (
                commandName == "QUIT" ||
                commandName == "EXIT"
            ) {

                cout
                    << "Client requested "
                    << commandName
                    << "."
                    << endl;

                close(clientSocket);

                return;
            }
        }
    }


    close(clientSocket);
}


// ============================================================
// START SERVER
// ============================================================

void TCPServer::start() {

    sockaddr_in serverAddress{};


    serverAddress.sin_family =
        AF_INET;


    serverAddress.sin_addr.s_addr =
        INADDR_ANY;


    serverAddress.sin_port =
        htons(port);


    // IMPORTANT:
    // ::bind avoids conflict with std::bind
    if (
        ::bind(
            serverSocket,
            reinterpret_cast<struct sockaddr*>(
                &serverAddress
            ),
            sizeof(serverAddress)
        ) < 0
    ) {

        cerr << "Bind failed!"
             << endl;

        close(serverSocket);

        exit(1);
    }


    // Start listening
    if (
        listen(
            serverSocket,
            10
        ) < 0
    ) {

        cerr << "Listen failed!"
             << endl;

        close(serverSocket);

        exit(1);
    }


    cout
        << "MiniRedis server started on port "
        << port
        << endl;


    // Accept clients forever
    while (true) {

        sockaddr_in clientAddress{};

        socklen_t clientLength =
            sizeof(clientAddress);


        int clientSocket =
            accept(
                serverSocket,
                reinterpret_cast<struct sockaddr*>(
                    &clientAddress
                ),
                &clientLength
            );


        if (clientSocket < 0) {

            cerr << "Accept failed!"
                 << endl;

            continue;
        }


        // Create separate thread
        // for every client
        thread clientThread(
            &TCPServer::handleClient,
            this,
            clientSocket
        );


        clientThread.detach();
    }
}