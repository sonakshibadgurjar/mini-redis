#ifndef COMMAND_H
#define COMMAND_H

#include <string>

#include "../database/database.h"

using namespace std;

class CommandHandler {

private:

    Database& db;

public:

    CommandHandler(Database& database);

    string execute(string input);
};

#endif