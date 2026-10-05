#ifndef RESP_H
#define RESP_H

#include <string>
#include <vector>

using namespace std;

class RESP {

public:

    // RESP response helpers
    static string simpleString(const string& value);

    static string error(const string& value);

    static string integer(long long value);

    static string bulkString(const string& value);

    static string nullString();

    static string array(
        const vector<string>& values
    );

    // Parse a command received from client
    static vector<string> parseCommand(
        const string& input
    );
};

#endif