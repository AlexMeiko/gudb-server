#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @brief 跳表节点，按 (score, value) 排序
 */
struct ZSkipListNode {
    double score_;
    std::string value_;
    std::unique_ptr<ZSkipListNode> next_;
    ZSkipListNode *down_;
    int span_; /// 跨度
    ZSkipListNode(double score, const std::string &value, std::unique_ptr<ZSkipListNode> next, ZSkipListNode *down,
                  int span = 1) : score_(score), value_(value), next_(std::move(next)), down_(down), span_(span) {}
};

/**
 * @brief Redis 风格的有序集跳表（支持按 score/lex 排序键）
 */
class ZSkipList {
private:
    int level_, size_; /// 当前层高、元素数
    std::vector<std::unique_ptr<ZSkipListNode>> headers_; /// 各层头节点，headers_.back() 为顶层
    std::vector<ZSkipListNode *> path_; /// 记录每层的前驱节点
    std::vector<int> rank_; /// 记录每层到前驱时跨越的底层元素数
    std::unordered_map<std::string, double> dict_; /// member -> score 映射

    /**
     * @brief 计算分数界限的排名 O(log N)
     * @param score 分数上界
     * @param inclusive true 统计 <= score，false 统计 < score
     * @return 底层中不大于界限的元素个数
     */
    int rankByScore(double score, bool inclusive) const;

    /**
     * @brief 计算按 (score,value) 组合键的排名 O(log N)
     * @param score 目标分数
     * @param value 目标成员值（分数相同时按字典序比较）
     * @param inclusive true 统计 <= (score,value)，false 统计 < (score,value)
     * @return 底层中小于（或小于等于）目标键的元素个数
     */
    int rankByKey(double score, const std::string &value, bool inclusive) const;

    /**
     * @brief 判断节点键是否小于目标键
     * @param a 当前节点
     * @param score 目标分数
     * @param value 目标值（分数相同时按字典序比较）
     * @return true 表示 a < (score,value)
     */
    bool less(const ZSkipListNode &a, const double &score, const std::string &value) const;

    /**
     * @brief 查找目标键的前驱（最底层）
     * @param score 目标分数
     * @param value 目标值（默认空串，仅按分数定位）
     * @return 最底层小于目标键的最后一个节点
     */
    ZSkipListNode *findPredecessor(const double &score, const std::string &value = "") const;

    /**
     * @brief 内部插入实现，不做去重检查
     * @param score 分数
     * @param value 成员值
     */
    void insert(double score, const std::string &value);

public:
    /**
     * @brief 构造函数，初始化空跳表
     */
    ZSkipList();

    /**
     * @brief 析构函数（依赖 unique_ptr 自动释放节点）
     */
    ~ZSkipList();

    ZSkipList(const ZSkipList &) = delete;
    ZSkipList &operator=(const ZSkipList &) = delete;
    ZSkipList(ZSkipList &&) noexcept = default;
    ZSkipList &operator=(ZSkipList &&) noexcept = default;

    /**
     * @brief 按成员值删除
     * @param value 成员值
     * @return true 表示找到并删除
     */
    bool eraseByValue(const std::string &value);

    /**
     * @brief 按索引闭区间获取成员列表
     * @param l 最小索引（包含，0-based）
     * @param r 最大索引（包含，0-based）
     * @param result 输出：按分数升序的成员值列表
     * @return 返回写入的元素个数
     */
    int getRange(int l, int r, std::vector<std::string> &result);

    /**
     * @brief 按字典序闭区间获取成员列表,需保证Score都一致且大于0
     * @param minValue 最小成员值（包含）
     * @param maxValue 最大成员值（包含）
     * @param result 输出：按字典序升序的成员值列表
     * @return 返回写入的元素个数
     */
    int getRangeByLex(const std::string &minValue, const std::string &maxValue, std::vector<std::string> &result);

    struct LexBound {
        enum class Type { NEG_INF, POS_INF, VALUE };
        Type type = Type::VALUE;
        bool inclusive = true;
        std::string value;
    };

    /**
     * @brief 按字典序区间统计元素数量（O(log N)，支持开/闭区间）
     * @param min 最小边界（-、[a、(a）
     * @param max 最大边界（+、[z、(z）
     * @return 区间内元素个数
     */
    int countByLex(const LexBound &min, const LexBound &max) const;

    /**
     * @brief 按分数区间获取成员列表（支持开/闭区间）
     * @param minScore 最小分数
     * @param minInclusive true 表示包含 minScore；false 表示排除 minScore
     * @param maxScore 最大分数
     * @param maxInclusive true 表示包含 maxScore；false 表示排除 maxScore
     * @param result 输出：按分数升序的成员值列表
     * @return 返回写入的元素个数
     */
    int getRangeByScore(double minScore, bool minInclusive, double maxScore, bool maxInclusive,
                        std::vector<std::string> &result);

    /**
     * @brief 按分数区间统计元素数量（O(log N)，支持开/闭区间）
     * @param minScore 最小分数
     * @param minInclusive true 表示包含 minScore；false 表示排除 minScore
     * @param maxScore 最大分数
     * @param maxInclusive true 表示包含 maxScore；false 表示排除 maxScore
     * @return 区间内元素个数
     */
    int countByScore(double minScore, bool minInclusive, double maxScore, bool maxInclusive) const;

    /**
     * @brief 插入或更新成员
     * @param score 新分数
     * @param value 成员值
     * @return true 表示新插入；false 表示已存在且分数未变或被更新
     */
    bool insertOrUpdate(double score, const std::string &value);

    /**
     * @brief 查询指定成员的分数（平均 O(1)）
     * @param value 成员值
     * @param result 输出：成员分数
     * @return true 表示存在并写入 result；false 表示不存在
     */
    bool getScore(const std::string &value, double &result) const;

    /**
     * @brief 增加指定成员的分数（不存在则以 0 为初始分数）
     * @param value 成员值
     * @param increment 增量
     * @return 更新后的新分数
     */
    double incrBy(const std::string &value, double increment);

    /**
     * @brief 当前元素个数
     * @return 元素总数
     */
    int size() const;
};
