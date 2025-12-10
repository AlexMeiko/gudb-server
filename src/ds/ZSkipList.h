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

    /**
     * @brief 按分数闭区间获取成员列表
     * @param minScore 最小分数（包含）
     * @param maxScore 最大分数（包含）
     * @param result 输出：按分数升序的成员值列表
     * @return 返回写入的元素个数
     */
    int getRangeByScore(double minScore, double maxScore, std::vector<std::string> &result);

    /**
     * @brief 插入或更新成员
     * @param score 新分数
     * @param value 成员值
     * @return true 表示新插入；false 表示已存在且分数未变或被更新
     */
    bool insertOrUpdate(double score, const std::string &value);

    /**
     * @brief 当前元素个数
     * @return 元素总数
     */
    int size() const;
};
