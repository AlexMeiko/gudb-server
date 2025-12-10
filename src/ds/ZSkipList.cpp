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

ZSkipListNode *ZSkipList::findPredecessor(const double &score, const std::string &value) const {
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
    ZSkipListNode *p = headers_.back().get();
    int cnt = 0; // 当前节点的底层索引
    while (p) {
        while (p->next_ && cnt + p->span_ <= l) {
            cnt += p->span_;
            p = p->next_.get();
        }
        if (p->down_) {
            p = p->down_;
        } else {
            break;
        }
    }

    for (p = p ? p->next_.get() : nullptr; l <= r && p; ++l, p = p->next_.get()) {
        result.push_back(p->value_);
    }

    return static_cast<int>(result.size());
}


int ZSkipList::getRangeByLex(const std::string &minValue, const std::string &maxValue,
                             std::vector<std::string> &result) {
    result.clear();
    ZSkipListNode *p = findPredecessor(0.0, minValue);

    for (p = p ? p->next_.get() : nullptr; p && p->value_ <= maxValue; p = p->next_.get()) {
        result.push_back(p->value_);
    }

    return static_cast<int>(result.size());
}

int ZSkipList::getRangeByScore(double minScore, double maxScore, std::vector<std::string> &result) {
    result.clear();
    ZSkipListNode *p = findPredecessor(minScore);

    for (p = p ? p->next_.get() : nullptr; p && p->score_ <= maxScore; p = p->next_.get()) {
        result.push_back(p->value_);
    }

    return static_cast<int>(result.size());
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

int ZSkipList::size() const { return size_; }
