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

    explicit Object(GString s) : value(std::move(s)), type(ObjType::STRING) {}

    explicit Object(GList l) : value(std::move(l)), type(ObjType::LIST) {}

    explicit Object(GHash h) : value(std::move(h)), type(ObjType::HASH) {}

    explicit Object(GSet s) : value(std::move(s)), type(ObjType::SET) {}

    explicit Object(GZSet s) : value(std::move(s)), type(ObjType::ZSET) {}
};

#endif //GUDB_OBJECT_H
