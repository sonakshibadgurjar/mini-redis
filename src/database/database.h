#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <unordered_map>
#include <vector>
#include <deque>
#include <unordered_set>
#include <chrono>
#include <mutex>

using namespace std;

class Database {

private:

    unordered_map<string, string> data;

    unordered_map<string, deque<string>> lists;

    unordered_map<string, unordered_set<string>> sets;

    unordered_map<string, unordered_map<string, string>> hashes;

    unordered_map<string, chrono::steady_clock::time_point> expiry;

    mutable mutex dbMutex;

    void removeExpired(const string& key);

public:

    // Strings
    void set(string key, string value);
    string get(string key);
    bool del(string key);
    bool exists(string key);
    vector<string> keys();

    // Lists
    int lpush(string key, string value);
    int rpush(string key, string value);
    string lpop(string key);
    string rpop(string key);
    vector<string> lrange(string key, int start, int stop);

    // Sets
    int sadd(string key, string value);
    int srem(string key, string value);
    bool sismember(string key, string value);
    vector<string> smembers(string key);

    // Hashes
    int hset(string key, string field, string value);
    string hget(string key, string field);
    int hdel(string key, string field);
    vector<string> hgetall(string key);

    // TTL
    bool expire(string key, int seconds);
    long long ttl(string key);

    // Persistence
    bool save(string filename);
    bool load(string filename);
};

#endif