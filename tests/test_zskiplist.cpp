#include <catch2/catch_test_macros.hpp>

#include "ds/ZSkipList.h"

#include <algorithm>
#include <limits>
#include <random>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "test_utils.h"

namespace {

    // 分数排序，分数相同按值字典序排列，模拟跳表顺序。
    struct ScoreValueCompare {
        bool operator()(const std::pair<double, std::string> &a, const std::pair<double, std::string> &b) const {
            if (a.first == b.first) {
                return a.second < b.second;
            }
            return a.first < b.first;
        }
    };

    // 参考模型：有序集合 + 快速查找字典，用来校验跳表行为。
    struct ReferenceState {
        std::set<std::pair<double, std::string>, ScoreValueCompare> ordered;
        std::unordered_map<std::string, double> dict;
    };

    // 取出当前有序序列的值列表。
    std::vector<std::string> orderedValues(const ReferenceState &ref) {
        std::vector<std::string> out;
        out.reserve(ref.ordered.size());
        for (const auto &kv: ref.ordered) {
            out.push_back(kv.second);
        }
        return out;
    }

    // 分数范围查询的参考实现。
    std::vector<std::string> rangeByScore(const ReferenceState &ref, double minScore, double maxScore) {
        std::vector<std::string> out;
        auto it = ref.ordered.lower_bound({minScore, ""});
        for (; it != ref.ordered.end() && it->first <= maxScore; ++it) {
            out.push_back(it->second);
        }
        return out;
    }

    // 按下标范围截取的参考实现。
    std::vector<std::string> rangeByIndex(const ReferenceState &ref, int l, int r) {
        std::vector<std::string> ordered = orderedValues(ref);
        if (ordered.empty()) {
            return {};
        }

        int n = static_cast<int>(ordered.size());
        l = std::max(0, l);
        r = std::min(r, n - 1);
        if (l > r) {
            return {};
        }

        return {ordered.begin() + l, ordered.begin() + r + 1};
    }

    // 仅在同分数下按值做字典序范围查询的参考实现。
    std::vector<std::string> rangeByLex(const ReferenceState &ref, const std::string &minValue,
                                        const std::string &maxValue, double score) {
        std::vector<std::string> out;
        auto it = ref.ordered.lower_bound({score, minValue});
        for (; it != ref.ordered.end() && it->first == score && it->second <= maxValue; ++it) {
            out.push_back(it->second);
        }
        return out;
    }

    // 从已存在的键中随机挑选一个值。
    std::string pickExistingValue(const ReferenceState &ref, test_utils::RandomSource &random) {
        auto it = ref.dict.begin();
        std::advance(it, random.randomIndex(0, static_cast<int>(ref.dict.size()) - 1));
        return it->first;
    }

    // 全量遍历校验跳表与参考模型顺序一致。
    void verifyFullOrder(ZSkipList &list, const ReferenceState &ref, const std::string &label) {
        std::vector<std::string> actual;
        list.getRangeByScore(std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max(), actual);
        REQUIRE(actual == orderedValues(ref));
        INFO(label);
    }

    // 随机混合插入/更新、删除、分数范围、下标范围操作，持续校验状态。
    void randomScoreOpsTest(int operations, unsigned seed) {
        test_utils::RandomSource random(seed);
        ZSkipList list;
        ReferenceState ref;
        int insertOrUpdateCnt = 0;
        int eraseCnt = 0;
        int scoreRangeCnt = 0;
        int indexRangeCnt = 0;

        for (int i = 1; i <= operations; ++i) {
            int action = random.randomIndex(0, 3); // 0 insert/update, 1 erase, 2 score range, 3 index range

            if (action == 0) {
                ++insertOrUpdateCnt;
                bool reuseExisting = !ref.dict.empty() && random.chance(0.55);
                std::string value = reuseExisting ? pickExistingValue(ref, random) : random.randomString();
                double score = random.randomScore();

                bool isNew = ref.dict.find(value) == ref.dict.end();
                bool sameScore = !isNew && ref.dict[value] == score;

                if (isNew) {
                    ref.ordered.insert({score, value});
                    ref.dict[value] = score;
                } else if (!sameScore) {
                    double oldScore = ref.dict[value];
                    ref.ordered.erase({oldScore, value});
                    ref.ordered.insert({score, value});
                    ref.dict[value] = score;
                }

                bool got = list.insertOrUpdate(score, value);
                INFO("op " << i << " seed " << seed << " value " << value << " score " << score);
                REQUIRE(got == isNew);
            } else if (action == 1) {
                ++eraseCnt;
                if (ref.dict.empty()) {
                    continue;
                }
                bool pickExisting = random.chance(0.6);
                std::string value = pickExisting ? pickExistingValue(ref, random) : random.randomString();

                bool expectedFound = false;
                auto it = ref.dict.find(value);
                if (it != ref.dict.end()) {
                    expectedFound = true;
                    ref.ordered.erase({it->second, value});
                    ref.dict.erase(it);
                }

                bool got = list.eraseByValue(value);
                INFO("op " << i << " seed " << seed << " erase value " << value);
                REQUIRE(got == expectedFound);
            } else if (action == 2) {
                ++scoreRangeCnt;
                double a = random.randomScore();
                double b = random.randomScore();
                double lo = std::min(a, b);
                double hi = std::max(a, b);

                auto expected = rangeByScore(ref, lo, hi);
                std::vector<std::string> actual;
                list.getRangeByScore(lo, hi, actual);
                INFO("op " << i << " seed " << seed << " score range [" << lo << ", " << hi << "]");
                REQUIRE(actual == expected);
            } else {
                ++indexRangeCnt;
                int maxIndex = static_cast<int>(ref.dict.size()) + 15;
                int l = random.randomIndex(-5, maxIndex);
                int r = random.randomIndex(-5, maxIndex);
                if (l > r) {
                    std::swap(l, r);
                }

                auto expected = rangeByIndex(ref, l, r);
                std::vector<std::string> actual;
                list.getRange(l, r, actual);
                INFO("op " << i << " seed " << seed << " index range [" << l << ", " << r << "]");
                REQUIRE(actual == expected);
            }

            INFO("op " << i << " seed " << seed);
            REQUIRE(list.size() == static_cast<int>(ref.dict.size()));

            if (i % 50 == 0) {
                verifyFullOrder(list, ref, "full order after " + std::to_string(i));
            }
        }

        INFO("seed " << seed << " insert/update " << insertOrUpdateCnt << " erase " << eraseCnt << " scoreRange "
                     << scoreRangeCnt << " indexRange " << indexRangeCnt);
    }

    // 在固定分数下随机生成值，测试字典序范围查询的正确性。
    void randomLexRangeTest(int valueCount, int queryCount, unsigned seed) {
        test_utils::RandomSource random(seed);
        ZSkipList list;
        ReferenceState ref;
        constexpr double lexScore = 0.0;

        for (int i = 0; i < valueCount; ++i) {
            std::string value;
            do {
                value = random.randomString(2, 10);
            } while (ref.dict.find(value) != ref.dict.end());

            ref.ordered.insert({lexScore, value});
            ref.dict[value] = lexScore;
            bool inserted = list.insertOrUpdate(lexScore, value);
            INFO("seed " << seed << " insert " << value);
            REQUIRE(inserted);
        }

        verifyFullOrder(list, ref, "lex dataset full order");

        std::vector<std::string> existing = orderedValues(ref);
        std::vector<std::string> actual;
        for (int q = 0; q < queryCount; ++q) {
            std::string a = existing[random.randomIndex(0, static_cast<int>(existing.size()) - 1)];
            std::string b = random.randomString(2, 20);

            std::string lo = a < b ? a : b;
            std::string hi = a < b ? b : a;

            auto expected = rangeByLex(ref, lo, hi, lexScore);
            list.getRangeByLex(lo, hi, actual);
            INFO("seed " << seed << " lex range [" << lo << ", " << hi << "]");
            REQUIRE(actual == expected);
        }

        const std::string &minValue = existing.front();
        const std::string &maxValue = existing.back() + "~";
        auto expectedAll = rangeByLex(ref, minValue, maxValue, lexScore);
        list.getRangeByLex(minValue, maxValue, actual);
        INFO("seed " << seed << " lex full span");
        REQUIRE(actual == expectedAll);
    }

} // namespace

TEST_CASE("skip list randomized mixed score operations", "[zskiplist][score]") {
    const unsigned seed = test_utils::chooseSeed();
    INFO("SKIPLIST_SEED=" << seed);
    randomScoreOpsTest(500, seed);
}

TEST_CASE("skip list lexicographical ranges with equal scores", "[zskiplist][lex]") {
    const unsigned seed = test_utils::chooseSeed() ^ 0x9E3779B9u;
    INFO("SKIPLIST_SEED=" << seed);
    randomLexRangeTest(80, 120, seed);
}
