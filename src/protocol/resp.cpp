#include "resp.h"

#include <sstream>
#include <stdexcept>

using namespace std;


// ============================================================
// SIMPLE STRING
// ============================================================

string RESP::simpleString(
    const string& value
) {

    return "+" +
           value +
           "\r\n";
}


// ============================================================
// ERROR
// ============================================================

string RESP::error(
    const string& value
) {

    return "-" +
           value +
           "\r\n";
}


// ============================================================
// INTEGER
// ============================================================

string RESP::integer(
    long long value
) {

    return ":" +
           to_string(value) +
           "\r\n";
}


// ============================================================
// BULK STRING
// ============================================================

string RESP::bulkString(
    const string& value
) {

    return "$" +
           to_string(value.size()) +
           "\r\n" +
           value +
           "\r\n";
}


// ============================================================
// NULL BULK STRING
// ============================================================

string RESP::nullString() {

    return "$-1\r\n";
}


// ============================================================
// ARRAY
// ============================================================

string RESP::array(
    const vector<string>& values
) {

    string response =
        "*" +
        to_string(values.size()) +
        "\r\n";


    for (const string& value : values) {

        response +=
            bulkString(value);
    }


    return response;
}


// ============================================================
// PARSE COMMAND
// ============================================================

vector<string> RESP::parseCommand(
    const string& input
) {

    vector<string> result;


    if (input.empty()) {
        return result;
    }


    // --------------------------------------------------------
    // RESP ARRAY FORMAT
    //
    // Example:
    //
    // *2\r\n
    // $4\r\n
    // PING\r\n
    // $4\r\n
    // test\r\n
    // --------------------------------------------------------

    if (input[0] == '*') {

        size_t position =
            input.find("\r\n");

        if (position == string::npos) {
            return result;
        }


        int count =
            stoi(
                input.substr(
                    1,
                    position - 1
                )
            );


        size_t index =
            position + 2;


        for (
            int i = 0;
            i < count;
            i++
        ) {

            if (
                index >= input.size() ||
                input[index] != '$'
            ) {
                return {};
            }


            size_t lengthEnd =
                input.find(
                    "\r\n",
                    index
                );


            if (
                lengthEnd ==
                string::npos
            ) {
                return {};
            }


            int length =
                stoi(
                    input.substr(
                        index + 1,
                        lengthEnd - index - 1
                    )
                );


            index =
                lengthEnd + 2;


            if (
                index + length >
                input.size()
            ) {
                return {};
            }


            result.push_back(
                input.substr(
                    index,
                    length
                )
            );


            index += length;


            if (
                index + 1 >=
                input.size()
            ) {
                return {};
            }


            if (
                input[index] != '\r' ||
                input[index + 1] != '\n'
            ) {
                return {};
            }


            index += 2;
        }


        return result;
    }


    // --------------------------------------------------------
    // INLINE COMMAND
    //
    // Example:
    //
    // SET name Sonakshi
    // --------------------------------------------------------

    string cleaned = input;


    while (
        !cleaned.empty() &&
        (
            cleaned.back() == '\r' ||
            cleaned.back() == '\n'
        )
    ) {

        cleaned.pop_back();
    }


    stringstream ss(cleaned);

    string word;


    while (ss >> word) {

        result.push_back(word);
    }


    return result;
}