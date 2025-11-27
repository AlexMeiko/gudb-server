#pragma once
#include "Object.h"
#include <unordered_map>
#include <string>
#include <chrono>

namespace gudb {
    class Database {
    public:
        Database() = default;

        // 检查键是否存在（读取类命令使用）
        bool exists(const std::string &key) {
            return expireIfNeeded(key) == false && data_.find(key) != data_.end();
        }

        // 获取值（如果键不存在或已过期，返回 nullptr）
        // 用于读取类命令：GET、HGET、ZSCORE 等
        Object *get(const std::string &key) {
            if (expireIfNeeded(key)) {
                return nullptr;
            }
            auto it = data_.find(key);
            if (it == data_.end()) {
                return nullptr;
            }
            return &it->second;
        }


        // 获取或创建（用于写入类命令）, 如果键不存在或类型不匹配，创建新对象
        Object &getOrCreate(const std::string &key, ObjType type) {
            expireIfNeeded(key);
            auto &obj = data_[key];
            if (obj.type != type) {
                // 类型不匹配或对象不存在，重新构造
                switch (type) {
                    case ObjType::STRING:
                        obj = Object(GString{});
                        break;
                    case ObjType::LIST:
                        obj = Object(GList{});
                        break;
                    case ObjType::HASH:
                        obj = Object(GHash{});
                        break;
                    case ObjType::SET:
                        obj = Object(GSet{});
                        break;
                    case ObjType::ZSET:
                        obj = Object(GZSet{});
                        break;
                }
            }
            return obj;
        }

        // 设置值（用于类型不匹配时的覆盖）
        void set(const std::string &key, Object obj) {
            data_[key] = std::move(obj);
        }

        // 删除键
        bool remove(const std::string &key) {
            return data_.erase(key) > 0;
        }

        // 获取数据库大小
        size_t size() const { return data_.size(); }

        // 清空数据库
        void clear() { data_.clear(); }

        // 设置过期时间
        void setExpire(const std::string &key, long long timestamp) {
            auto it = data_.find(key);
            if (it != data_.end()) {
                it->second.expiresAt = timestamp;
            }
        }

        // 获取过期时间
        long long getExpire(const std::string &key) const {
            auto it = data_.find(key);
            if (it == data_.end()) {
                return -1;
            }
            return it->second.expiresAt;
        }

        // 移除过期时间
        void persist(const std::string &key) {
            auto it = data_.find(key);
            if (it != data_.end()) {
                it->second.expiresAt = -1;
            }
        }

    private:
        std::unordered_map<std::string, Object> data_;

        // 检查对象是否过期
        bool isExpired(const Object &obj) const {
            if (obj.expiresAt == -1) {
                return false;
            }
            auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count();
            return now > obj.expiresAt;
        }

        // 惰性删除：如果过期则删除，返回是否已过期
        bool expireIfNeeded(const std::string &key) {
            auto it = data_.find(key);
            if (it == data_.end()) {
                return false;
            }
            if (isExpired(it->second)) {
                data_.erase(it);
                return true;
            }
            return false;
        }
    };
}
