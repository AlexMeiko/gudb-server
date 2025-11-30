#ifndef GUDB_OBJECT_H
#define GUDB_OBJECT_H

#include <string>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <variant>
#include <iostream>

using GString = std::string;
using GList = std::deque<std::string>;
using GHash = std::unordered_map<std::string, std::string>;
using GSet = std::unordered_set<std::string>;

struct GZSet {
    std::unordered_map<std::string, double> dict;
    std::set<std::pair<double, std::string> > zsl;
};

enum class ObjType { STRING, LIST, HASH, SET, ZSET };

class Object {
public:
    std::variant<GString, GList, GHash, GSet, GZSet> value;
    ObjType type;
    long long expiresAt = -1; // 过期时间戳（毫秒），-1 表示永不过期

    Object() : value(GString{}), type(ObjType::STRING) {}

    explicit Object(GString s, long long expire = -1) : value(std::move(s)), type(ObjType::STRING), expiresAt(expire) {}

    explicit Object(GList l, long long expire = -1) : value(std::move(l)), type(ObjType::LIST), expiresAt(expire) {}

    explicit Object(GHash h, long long expire = -1) : value(std::move(h)), type(ObjType::HASH), expiresAt(expire) {}

    explicit Object(GSet s, long long expire = -1) : value(std::move(s)), type(ObjType::SET), expiresAt(expire) {}

    explicit Object(GZSet s, long long expire = -1) : value(std::move(s)), type(ObjType::ZSET), expiresAt(expire) {}
};

#endif //GUDB_OBJECT_H