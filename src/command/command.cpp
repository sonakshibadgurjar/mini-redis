#include "command.h"
#include "../protocol/resp.h"

#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

using namespace std;


// ============================================================
// HELPER FUNCTIONS
// ============================================================

// Convert command to uppercase
static string toUpperCase(string str) {

    for (char& c : str) {

        c = static_cast<char>(
            toupper(
                static_cast<unsigned char>(c)
            )
        );
    }

    return str;
}


// Create RESP array from strings
static string makeArray(
    const vector<string>& values
) {

    string response =
        "*" + to_string(values.size()) + "\r\n";

    for (const string& value : values) {

        response +=
            "$" +
            to_string(value.size()) +
            "\r\n";

        response += value;
        response += "\r\n";
    }

    return response;
}


// Create RESP integer
static string makeInteger(long long value) {

    return ":" +
           to_string(value) +
           "\r\n";
}


// Create RESP bulk string
static string makeBulkString(
    const string& value
) {

    return "$" +
           to_string(value.size()) +
           "\r\n" +
           value +
           "\r\n";
}


// Create RESP NULL bulk string
static string makeNull() {

    return "$-1\r\n";
}


// ============================================================
// CONSTRUCTOR
// ============================================================

CommandHandler::CommandHandler(Database& database)
    : db(database) {
}


// ============================================================
// EXECUTE COMMAND
// ============================================================

string CommandHandler::execute(string input) {

    // Remove \r and \n
    while (
        !input.empty() &&
        (
            input.back() == '\r' ||
            input.back() == '\n'
        )
    ) {

        input.pop_back();
    }


    if (input.empty()) {

        return RESP::error(
            "ERR empty command"
        );
    }


    // --------------------------------------------------------
    // Extract command
    // --------------------------------------------------------

    stringstream ss(input);

    string command;

    ss >> command;

    command = toUpperCase(command);


    // ========================================================
    // PING
    // ========================================================

    if (command == "PING") {

        return RESP::simpleString("PONG");
    }


    // ========================================================
    // QUIT
    // ========================================================

    if (command == "QUIT") {

        return RESP::simpleString("BYE");
    }


    // ========================================================
    // EXIT
    // ========================================================

    if (command == "EXIT") {

        return RESP::simpleString("BYE");
    }


    // ========================================================
    // SET
    // ========================================================

    if (command == "SET") {

        string key;

        ss >> key;

        if (key.empty()) {

            return RESP::error(
                "ERR wrong number of arguments for SET"
            );
        }


        // Preserve spaces in value
        string value;

        getline(ss, value);


        // Remove leading spaces
        size_t first =
            value.find_first_not_of(' ');

        if (first != string::npos) {

            value =
                value.substr(first);

        } else {

            value = "";
        }


        db.set(key, value);

        return RESP::simpleString("OK");
    }


    // ========================================================
    // GET
    // ========================================================

    if (command == "GET") {

        string key;

        ss >> key;

        if (key.empty()) {

            return RESP::error(
                "ERR wrong number of arguments for GET"
            );
        }


        string value =
            db.get(key);


        if (value == "(nil)") {

            return makeNull();
        }


        return makeBulkString(value);
    }


    // ========================================================
    // DEL
    // ========================================================

    if (command == "DEL") {

        string key;

        ss >> key;

        if (key.empty()) {

            return RESP::error(
                "ERR wrong number of arguments for DEL"
            );
        }


        bool removed =
            db.del(key);


        return makeInteger(
            removed ? 1 : 0
        );
    }


    // ========================================================
    // EXISTS
    // ========================================================

    if (command == "EXISTS") {

        string key;

        ss >> key;

        if (key.empty()) {

            return RESP::error(
                "ERR wrong number of arguments for EXISTS"
            );
        }


        return makeInteger(
            db.exists(key) ? 1 : 0
        );
    }


    // ========================================================
    // KEYS
    // ========================================================

    if (command == "KEYS") {

        string pattern;

        ss >> pattern;


        // Current implementation supports *
        // as the main pattern.
        if (
            !pattern.empty() &&
            pattern != "*"
        ) {

            return makeArray({});
        }


        vector<string> result =
            db.keys();


        return makeArray(result);
    }


    // ========================================================
    // LPUSH
    // ========================================================

    if (command == "LPUSH") {

        string key;
        string value;

        ss >> key;
        ss >> value;


        if (
            key.empty() ||
            value.empty()
        ) {

            return RESP::error(
                "ERR wrong number of arguments for LPUSH"
            );
        }


        int size =
            db.lpush(
                key,
                value
            );


        return makeInteger(size);
    }


    // ========================================================
    // RPUSH
    // ========================================================

    if (command == "RPUSH") {

        string key;
        string value;

        ss >> key;
        ss >> value;


        if (
            key.empty() ||
            value.empty()
        ) {

            return RESP::error(
                "ERR wrong number of arguments for RPUSH"
            );
        }


        int size =
            db.rpush(
                key,
                value
            );


        return makeInteger(size);
    }


    // ========================================================
    // LPOP
    // ========================================================

    if (command == "LPOP") {

        string key;

        ss >> key;


        if (key.empty()) {

            return RESP::error(
                "ERR wrong number of arguments for LPOP"
            );
        }


        string value =
            db.lpop(key);


        if (value == "(nil)") {

            return makeNull();
        }


        return makeBulkString(value);
    }


    // ========================================================
    // RPOP
    // ========================================================

    if (command == "RPOP") {

        string key;

        ss >> key;


        if (key.empty()) {

            return RESP::error(
                "ERR wrong number of arguments for RPOP"
            );
        }


        string value =
            db.rpop(key);


        if (value == "(nil)") {

            return makeNull();
        }


        return makeBulkString(value);
    }


    // ========================================================
    // LRANGE
    // ========================================================

    if (command == "LRANGE") {

        string key;

        int start;
        int stop;


        if (!(ss >> key >> start >> stop)) {

            return RESP::error(
                "ERR wrong number of arguments for LRANGE"
            );
        }


        vector<string> result =
            db.lrange(
                key,
                start,
                stop
            );


        return makeArray(result);
    }


    // ========================================================
    // SADD
    // ========================================================

    if (command == "SADD") {

        string key;
        string value;

        ss >> key;
        ss >> value;


        if (
            key.empty() ||
            value.empty()
        ) {

            return RESP::error(
                "ERR wrong number of arguments for SADD"
            );
        }


        int added =
            db.sadd(
                key,
                value
            );


        return makeInteger(added);
    }


    // ========================================================
    // SREM
    // ========================================================

    if (command == "SREM") {

        string key;
        string value;

        ss >> key;
        ss >> value;


        if (
            key.empty() ||
            value.empty()
        ) {

            return RESP::error(
                "ERR wrong number of arguments for SREM"
            );
        }


        int removed =
            db.srem(
                key,
                value
            );


        return makeInteger(removed);
    }


    // ========================================================
    // SISMEMBER
    // ========================================================

    if (command == "SISMEMBER") {

        string key;
        string value;

        ss >> key;
        ss >> value;


        if (
            key.empty() ||
            value.empty()
        ) {

            return RESP::error(
                "ERR wrong number of arguments for SISMEMBER"
            );
        }


        bool member =
            db.sismember(
                key,
                value
            );


        return makeInteger(
            member ? 1 : 0
        );
    }


    // ========================================================
    // SMEMBERS
    // ========================================================

    if (command == "SMEMBERS") {

        string key;

        ss >> key;


        if (key.empty()) {

            return RESP::error(
                "ERR wrong number of arguments for SMEMBERS"
            );
        }


        vector<string> result =
            db.smembers(key);


        return makeArray(result);
    }


    // ========================================================
    // HSET
    // ========================================================

    if (command == "HSET") {

        string key;
        string field;
        string value;


        if (!(ss >> key >> field >> value)) {

            return RESP::error(
                "ERR wrong number of arguments for HSET"
            );
        }


        int added =
            db.hset(
                key,
                field,
                value
            );


        return makeInteger(added);
    }


    // ========================================================
    // HGET
    // ========================================================

    if (command == "HGET") {

        string key;
        string field;


        if (!(ss >> key >> field)) {

            return RESP::error(
                "ERR wrong number of arguments for HGET"
            );
        }


        string value =
            db.hget(
                key,
                field
            );


        if (value == "(nil)") {

            return makeNull();
        }


        return makeBulkString(value);
    }


    // ========================================================
    // HDEL
    // ========================================================

    if (command == "HDEL") {

        string key;
        string field;


        if (!(ss >> key >> field)) {

            return RESP::error(
                "ERR wrong number of arguments for HDEL"
            );
        }


        int removed =
            db.hdel(
                key,
                field
            );


        return makeInteger(removed);
    }


    // ========================================================
    // HGETALL
    // ========================================================

    if (command == "HGETALL") {

        string key;

        ss >> key;


        if (key.empty()) {

            return RESP::error(
                "ERR wrong number of arguments for HGETALL"
            );
        }


        vector<string> result =
            db.hgetall(key);


        return makeArray(result);
    }


    // ========================================================
    // EXPIRE
    // ========================================================

    if (command == "EXPIRE") {

        string key;

        int seconds;


        if (!(ss >> key >> seconds)) {

            return RESP::error(
                "ERR wrong number of arguments for EXPIRE"
            );
        }


        bool success =
            db.expire(
                key,
                seconds
            );


        return makeInteger(
            success ? 1 : 0
        );
    }


    // ========================================================
    // TTL
    // ========================================================

    if (command == "TTL") {

        string key;

        ss >> key;


        if (key.empty()) {

            return RESP::error(
                "ERR wrong number of arguments for TTL"
            );
        }


        long long remaining =
            db.ttl(key);


        return makeInteger(
            remaining
        );
    }


    // ========================================================
    // SAVE
    // ========================================================

    if (command == "SAVE") {

        bool success =
            db.save("dump.rdb");


        if (success) {

            return RESP::simpleString("OK");
        }


        return RESP::error(
            "ERR save failed"
        );
    }


    // ========================================================
    // LOAD
    // ========================================================

    if (command == "LOAD") {

        bool success =
            db.load("dump.rdb");


        if (success) {

            return RESP::simpleString("OK");
        }


        return RESP::error(
            "ERR load failed"
        );
    }


    // ========================================================
    // UNKNOWN COMMAND
    // ========================================================

    return RESP::error(
        "ERR unknown command"
    );
}