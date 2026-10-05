#include "database.h"

#include <fstream>
#include <sstream>

using namespace std;

void Database::removeExpired(const string& key) {

    auto it = expiry.find(key);

    if (it != expiry.end()) {

        if (chrono::steady_clock::now() >= it->second) {

            data.erase(key);
            lists.erase(key);
            sets.erase(key);
            hashes.erase(key);
            expiry.erase(it);
        }
    }
}

// ==================== STRING ====================

void Database::set(string key, string value) {

    lock_guard<mutex> lock(dbMutex);

    data[key] = value;

    expiry.erase(key);
}

string Database::get(string key) {

    lock_guard<mutex> lock(dbMutex);

    removeExpired(key);

    auto it = data.find(key);

    if (it == data.end()) {
        return "(nil)";
    }

    return it->second;
}

bool Database::del(string key) {

    lock_guard<mutex> lock(dbMutex);

    removeExpired(key);

    bool removed = false;

    if (data.erase(key)) removed = true;
    if (lists.erase(key)) removed = true;
    if (sets.erase(key)) removed = true;
    if (hashes.erase(key)) removed = true;

    expiry.erase(key);

    return removed;
}

bool Database::exists(string key) {

    lock_guard<mutex> lock(dbMutex);

    removeExpired(key);

    return data.count(key) ||
           lists.count(key) ||
           sets.count(key) ||
           hashes.count(key);
}

vector<string> Database::keys() {

    lock_guard<mutex> lock(dbMutex);

    vector<string> result;

    for (auto& p : data) {
        removeExpired(p.first);
    }

    for (auto& p : data) {
        result.push_back(p.first);
    }

    for (auto& p : lists) {
        result.push_back(p.first);
    }

    for (auto& p : sets) {
        result.push_back(p.first);
    }

    for (auto& p : hashes) {
        result.push_back(p.first);
    }

    return result;
}

// ==================== LIST ====================

int Database::lpush(string key, string value) {

    lock_guard<mutex> lock(dbMutex);

    removeExpired(key);

    lists[key].push_front(value);

    return lists[key].size();
}

int Database::rpush(string key, string value) {

    lock_guard<mutex> lock(dbMutex);

    removeExpired(key);

    lists[key].push_back(value);

    return lists[key].size();
}

string Database::lpop(string key) {

    lock_guard<mutex> lock(dbMutex);

    removeExpired(key);

    if (!lists.count(key) || lists[key].empty()) {
        return "(nil)";
    }

    string value = lists[key].front();

    lists[key].pop_front();

    if (lists[key].empty()) {
        lists.erase(key);
    }

    return value;
}

string Database::rpop(string key) {

    lock_guard<mutex> lock(dbMutex);

    removeExpired(key);

    if (!lists.count(key) || lists[key].empty()) {
        return "(nil)";
    }

    string value = lists[key].back();

    lists[key].pop_back();

    if (lists[key].empty()) {
        lists.erase(key);
    }

    return value;
}

vector<string> Database::lrange(
    string key,
    int start,
    int stop
) {

    lock_guard<mutex> lock(dbMutex);

    removeExpired(key);

    vector<string> result;

    if (!lists.count(key)) {
        return result;
    }

    auto& list = lists[key];

    int size = static_cast<int>(list.size());

    if (start < 0) start = size + start;
    if (stop < 0) stop = size + stop;

    if (start < 0) start = 0;

    if (stop >= size) stop = size - 1;

    if (start > stop || start >= size) {
        return result;
    }

    for (int i = start; i <= stop; i++) {
        result.push_back(list[i]);
    }

    return result;
}

// ==================== SET ====================

int Database::sadd(string key, string value) {

    lock_guard<mutex> lock(dbMutex);

    removeExpired(key);

    auto& s = sets[key];

    auto result = s.insert(value);

    return result.second ? 1 : 0;
}

int Database::srem(string key, string value) {

    lock_guard<mutex> lock(dbMutex);

    removeExpired(key);

    if (!sets.count(key)) {
        return 0;
    }

    int removed = sets[key].erase(value);

    if (sets[key].empty()) {
        sets.erase(key);
    }

    return removed;
}

bool Database::sismember(string key, string value) {

    lock_guard<mutex> lock(dbMutex);

    removeExpired(key);

    if (!sets.count(key)) {
        return false;
    }

    return sets[key].count(value) > 0;
}

vector<string> Database::smembers(string key) {

    lock_guard<mutex> lock(dbMutex);

    removeExpired(key);

    vector<string> result;

    if (!sets.count(key)) {
        return result;
    }

    for (auto& value : sets[key]) {
        result.push_back(value);
    }

    return result;
}

// ==================== HASH ====================

int Database::hset(
    string key,
    string field,
    string value
) {

    lock_guard<mutex> lock(dbMutex);

    removeExpired(key);

    bool isNew = !hashes[key].count(field);

    hashes[key][field] = value;

    return isNew ? 1 : 0;
}

string Database::hget(
    string key,
    string field
) {

    lock_guard<mutex> lock(dbMutex);

    removeExpired(key);

    if (!hashes.count(key)) {
        return "(nil)";
    }

    if (!hashes[key].count(field)) {
        return "(nil)";
    }

    return hashes[key][field];
}

int Database::hdel(
    string key,
    string field
) {

    lock_guard<mutex> lock(dbMutex);

    removeExpired(key);

    if (!hashes.count(key)) {
        return 0;
    }

    int removed = hashes[key].erase(field);

    if (hashes[key].empty()) {
        hashes.erase(key);
    }

    return removed;
}

vector<string> Database::hgetall(string key) {

    lock_guard<mutex> lock(dbMutex);

    removeExpired(key);

    vector<string> result;

    if (!hashes.count(key)) {
        return result;
    }

    for (auto& p : hashes[key]) {

        result.push_back(p.first);
        result.push_back(p.second);
    }

    return result;
}

// ==================== TTL ====================

bool Database::expire(string key, int seconds) {

    lock_guard<mutex> lock(dbMutex);

    bool exists =
        data.count(key) ||
        lists.count(key) ||
        sets.count(key) ||
        hashes.count(key);

    if (!exists) {
        return false;
    }

    if (seconds <= 0) {

        data.erase(key);
        lists.erase(key);
        sets.erase(key);
        hashes.erase(key);
        expiry.erase(key);

        return true;
    }

    expiry[key] =
        chrono::steady_clock::now() +
        chrono::seconds(seconds);

    return true;
}

long long Database::ttl(string key) {

    lock_guard<mutex> lock(dbMutex);

    bool exists =
        data.count(key) ||
        lists.count(key) ||
        sets.count(key) ||
        hashes.count(key);

    if (!exists) {
        return -2;
    }

    // Key exists but has no expiry
    if (!expiry.count(key)) {
        return -1;
    }

    auto now = chrono::steady_clock::now();
    auto expireTime = expiry[key];

    if (now >= expireTime) {

        data.erase(key);
        lists.erase(key);
        sets.erase(key);
        hashes.erase(key);
        expiry.erase(key);

        return -2;
    }

    // Calculate remaining time in milliseconds
    auto remainingMs =
        chrono::duration_cast<chrono::milliseconds>(
            expireTime - now
        ).count();

    // Round UP to the next second
    long long remainingSeconds =
        (remainingMs + 999) / 1000;

    return remainingSeconds;
}

// ==================== PERSISTENCE ====================

bool Database::save(string filename) {

    lock_guard<mutex> lock(dbMutex);

    ofstream file(filename);

    if (!file) {
        return false;
    }

    for (auto& p : data) {

        removeExpired(p.first);

        file << "STRING "
             << p.first << " "
             << p.second << "\n";
    }

    for (auto& p : lists) {

        removeExpired(p.first);

        for (auto& value : p.second) {

            file << "LIST "
                 << p.first << " "
                 << value << "\n";
        }
    }

    for (auto& p : sets) {

        removeExpired(p.first);

        for (auto& value : p.second) {

            file << "SET "
                 << p.first << " "
                 << value << "\n";
        }
    }

    for (auto& p : hashes) {

        removeExpired(p.first);

        for (auto& field : p.second) {

            file << "HASH "
                 << p.first << " "
                 << field.first << " "
                 << field.second << "\n";
        }
    }

    return true;
}

bool Database::load(string filename) {

    lock_guard<mutex> lock(dbMutex);

    ifstream file(filename);

    if (!file) {
        return false;
    }

    string type;

    while (file >> type) {

        if (type == "STRING") {

            string key;
            string value;

            file >> key >> value;

            data[key] = value;
        }

        else if (type == "LIST") {

            string key;
            string value;

            file >> key >> value;

            lists[key].push_back(value);
        }

        else if (type == "SET") {

            string key;
            string value;

            file >> key >> value;

            sets[key].insert(value);
        }

        else if (type == "HASH") {

            string key;
            string field;
            string value;

            file >> key >> field >> value;

            hashes[key][field] = value;
        }
    }

    return true;
}