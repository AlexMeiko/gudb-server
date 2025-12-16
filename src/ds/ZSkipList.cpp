#include "ZSkipList.h"
#include <algorithm>
#include <random>

namespace {

    constexpr int MAX_LEVEL = 32;

    std::mt19937 rng(std::random_device{}());
    /**
     * @brief 返回随机整数，用于决定节点是否提升层高
     */
    int randInt() { return rng(); }

    int randomLevel() {
        int lvl = 1;
        while ((randInt() & 1) == 0 && lvl < MAX_LEVEL) {
            ++lvl;
        }
        return lvl;
    }

} // namespace

bool ZSkipList::less(const ZSkipListNode &a, const double &score, const std::string &value) const {
    return a.score_ < score || (a.score_ == score && a.value_ < value);
}

ZSkipListNode *ZSkipList::findPredecessorByKey(const double &score, const std::string &value) const {
    ZSkipListNode *p = headers_.empty() ? nullptr : headers_.back().get();
    while (p) {
        while (p->next_ && less(*p->next_, score, value)) {
            p = p->next_.get();
        }
        if (p->down_) {
            p = p->down_;
        } else {
            break;
        }
    }
    return p;
}

ZSkipListNode *ZSkipList::findPredecessorByIndex(int index) const {
    ZSkipListNode *p = headers_.empty() ? nullptr : headers_.back().get();
    int cnt = 0; // 当前节点的底层索引
    while (p) {
        while (p->next_ && cnt + p->span_ <= index) {
            cnt += p->span_;
            p = p->next_.get();
        }
        if (p->down_) {
            p = p->down_;
        } else {
            break;
        }
    }
    return p;
}

void ZSkipList::insert(double score, const std::string &value) {
    path_.clear();
    rank_.clear();
    ZSkipListNode *p = headers_.empty() ? nullptr : headers_.back().get();
    int curRank = 0;

    // 获取路径并记录到前驱的 rank
    while (p) {
        while (p->next_ && less(*p->next_, score, value)) {
            curRank += p->span_;
            p = p->next_.get();
        }
        path_.push_back(p);
        rank_.push_back(curRank);
        p = p->down_;
    }

    int rankBottom = rank_.empty() ? 0 : rank_.back();

    // 随机层高
    int nodeLevel = std::min(randomLevel(), MAX_LEVEL);

    // 插入新节点（自底向上），拆分跨度；若层高超过当前高度，动态加顶层
    ZSkipListNode *downNode = nullptr;
    for (int i = 0; i < nodeLevel; ++i) {
        if (path_.empty()) {
            // 需要比当前更高的层，动态添加新顶层 header
            auto newHeader = std::make_unique<ZSkipListNode>(0, "", nullptr, headers_.back().get(), size_);
            headers_.push_back(std::move(newHeader));
            path_.push_back(headers_.back().get());
            rank_.push_back(0);
            ++level_;
        }

        ZSkipListNode *prev = path_.back();
        int offset = rankBottom - rank_.back();
        path_.pop_back();
        rank_.pop_back();

        auto newNode = std::make_unique<ZSkipListNode>(score, value, std::move(prev->next_), downNode);
        newNode->span_ = prev->span_ - offset;
        prev->span_ = offset + 1;

        prev->next_ = std::move(newNode);
        downNode = prev->next_.get();
    }

    // 未插入新节点的更高层跨度 +1
    while (!path_.empty()) {
        ++path_.back()->span_;
        path_.pop_back();
        rank_.pop_back();
    }

    ++size_;
}

ZSkipList::ZSkipList() : level_(1), size_(0) {
    headers_.push_back(std::make_unique<ZSkipListNode>(0, "", nullptr, nullptr, 0));
}

ZSkipList::~ZSkipList() = default;

int ZSkipList::rankByScore(double score, bool inclusive) const {
    ZSkipListNode *p = headers_.empty() ? nullptr : headers_.back().get();
    int cnt = 0;

    while (p) {
        while (p->next_ && (inclusive ? p->next_->score_ <= score : p->next_->score_ < score)) {
            cnt += p->span_;
            p = p->next_.get();
        }

        if (p->down_) {
            p = p->down_;
        } else {
            break;
        }
    }
    return cnt;
}

int ZSkipList::rankByKey(double score, const std::string &value, bool inclusive) const {
    ZSkipListNode *p = headers_.empty() ? nullptr : headers_.back().get();
    int cnt = 0;

    while (p) {
        while (p->next_ && (less(*p->next_, score, value) ||
                            (inclusive && p->next_->score_ == score && p->next_->value_ == value))) {
            cnt += p->span_;
            p = p->next_.get();
        }

        if (p->down_) {
            p = p->down_;
        } else {
            break;
        }
    }
    return cnt;
}

bool ZSkipList::eraseByValue(const std::string &value) {
    auto it = dict_.find(value);
    if (it == dict_.end()) {
        return false;
    }

    double score = it->second;
    dict_.erase(it);

    ZSkipListNode *p = headers_.empty() ? nullptr : headers_.back().get();
    bool found = false;
    while (p) {
        while (p->next_ && less(*p->next_, score, value)) {
            p = p->next_.get();
        }

        // 更新跨度
        --p->span_;

        if (p->next_ && p->next_->score_ == score && p->next_->value_ == value) {

            auto deletePtr = std::move(p->next_);
            p->span_ += deletePtr->span_; // 更新跨度
            p->next_ = std::move(deletePtr->next_);
            found = true;
        }

        p = p->down_;
    }

    size_ -= static_cast<int>(found);

    while (found && headers_.size() > 1 && !headers_.back()->next_) {
        headers_.pop_back();
        --level_;
    }

    return found;
}

int ZSkipList::getRange(int l, int r, std::vector<std::string> &result) {
    result.clear();
    l = std::max(0, l), r = std::min(r, size_ - 1);
    if (l > r || headers_.empty() || size_ == 0) {
        return 0;
    }

    // 定位到 l 的前驱，O(log N)
    ZSkipListNode *p = findPredecessorByIndex(l);

    for (p = p ? p->next_.get() : nullptr; l <= r && p; ++l, p = p->next_.get()) {
        result.push_back(p->value_);
    }

    return static_cast<int>(result.size());
}

int ZSkipList::getRange(int l, int r, std::vector<std::pair<double, std::string>> &result) {
    result.clear();
    l = std::max(0, l), r = std::min(r, size_ - 1);
    if (l > r || headers_.empty() || size_ == 0) {
        return 0;
    }

    // 定位到 l 的前驱，O(log N)
    ZSkipListNode *p = findPredecessorByIndex(l);

    for (p = p ? p->next_.get() : nullptr; l <= r && p; ++l, p = p->next_.get()) {
        result.emplace_back(p->score_, p->value_);
    }

    return static_cast<int>(result.size());
}

int ZSkipList::getRangeByLex(const std::string &minValue, const std::string &maxValue,
                             std::vector<std::string> &result) {
    result.clear();
    if (size_ == 0) {
        return 0;
    }

    const ZSkipListNode *first = headers_.front()->next_.get();
    if (!first) {
        return 0;
    }

    const double lexScore = first->score_;
    ZSkipListNode *p = findPredecessorByKey(lexScore, minValue);

    for (p = p ? p->next_.get() : nullptr; p && p->score_ == lexScore && p->value_ <= maxValue; p = p->next_.get()) {
        result.push_back(p->value_);
    }

    return static_cast<int>(result.size());
}

int ZSkipList::countByLex(const LexBound &min, const LexBound &max) const {
    if (size_ == 0) {
        return 0;
    }

    if (min.type == LexBound::Type::POS_INF || max.type == LexBound::Type::NEG_INF) {
        return 0;
    }

    if (min.type == LexBound::Type::VALUE && max.type == LexBound::Type::VALUE) {
        if (min.value > max.value || (min.value == max.value && (!min.inclusive || !max.inclusive))) {
            return 0;
        }
    }

    const ZSkipListNode *first = headers_.front()->next_.get();
    if (!first) {
        return 0;
    }

    const double lexScore = first->score_;
    const int l = min.type == LexBound::Type::VALUE ? rankByKey(lexScore, min.value, !min.inclusive) : 0;
    const int r = max.type == LexBound::Type::VALUE ? rankByKey(lexScore, max.value, max.inclusive)
                                                    : rankByScore(lexScore, true);
    return std::max(0, r - l);
}

int ZSkipList::getRangeByScore(double minScore, bool minInclusive, double maxScore, bool maxInclusive,
                               std::vector<std::string> &result) {
    result.clear();
    if (size_ == 0) {
        return 0;
    }

    if (minScore > maxScore || (minScore == maxScore && (!minInclusive || !maxInclusive))) {
        return 0;
    }

    ZSkipListNode *p = findPredecessorByKey(minScore);
    p = p ? p->next_.get() : nullptr;

    if (!minInclusive) {
        while (p && p->score_ == minScore) {
            p = p->next_.get();
        }
    }

    for (; p && (maxInclusive ? p->score_ <= maxScore : p->score_ < maxScore); p = p->next_.get()) {
        result.push_back(p->value_);
    }

    return static_cast<int>(result.size());
}

int ZSkipList::getRangeByScore(double minScore, bool minInclusive, double maxScore, bool maxInclusive,
                               std::vector<std::pair<double, std::string>> &result) {
    result.clear();
    if (size_ == 0) {
        return 0;
    }

    if (minScore > maxScore || (minScore == maxScore && (!minInclusive || !maxInclusive))) {
        return 0;
    }

    ZSkipListNode *p = findPredecessorByKey(minScore);
    p = p ? p->next_.get() : nullptr;

    if (!minInclusive) {
        while (p && p->score_ == minScore) {
            p = p->next_.get();
        }
    }

    for (; p && (maxInclusive ? p->score_ <= maxScore : p->score_ < maxScore); p = p->next_.get()) {
        result.emplace_back(p->score_, p->value_);
    }

    return static_cast<int>(result.size());
}

int ZSkipList::countByScore(double minScore, bool minInclusive, double maxScore, bool maxInclusive) const {
    if (size_ == 0) {
        return 0;
    }

    if (minScore > maxScore || (minScore == maxScore && (!minInclusive || !maxInclusive))) {
        return 0;
    }

    const int l = rankByScore(minScore, minInclusive ? false : true);
    const int r = rankByScore(maxScore, maxInclusive);
    return std::max(0, r - l);
}

bool ZSkipList::insertOrUpdate(double score, const std::string &value) {
    auto it = dict_.find(value);
    bool res = it == dict_.end();

    if (!res) {
        if (it->second == score) {
            return false;
        }
        eraseByValue(value);
    }

    dict_[value] = score;
    insert(score, value);
    return res;
}

bool ZSkipList::getScore(const std::string &value, double &result) const {
    auto it = dict_.find(value);
    if (it == dict_.end()) {
        return false;
    }

    result = it->second;
    return true;
}

double ZSkipList::incrBy(const std::string &value, double increment) {
    double oldScore = 0.0;
    if (auto it = dict_.find(value); it != dict_.end()) {
        oldScore = it->second;
    }

    double newScore = oldScore + increment;
    insertOrUpdate(newScore, value);
    return newScore;
}

int ZSkipList::size() const { return size_; }
