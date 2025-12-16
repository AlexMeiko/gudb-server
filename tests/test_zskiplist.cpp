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

    std::vector<std::pair<double, std::string>> orderedPairs(const ReferenceState &ref) {
        std::vector<std::pair<double, std::string>> out;
        out.reserve(ref.ordered.size());
        for (const auto &kv: ref.ordered) {
            out.push_back(kv);
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

    static inline bool inScoreRange(double score, double minScore, bool minInclusive, double maxScore,
                                    bool maxInclusive) {
        if (minScore > maxScore || (minScore == maxScore && (!minInclusive || !maxInclusive))) {
            return false;
        }
        if (score < minScore || score > maxScore) {
            return false;
        }
        if (!minInclusive && score == minScore) {
            return false;
        }
        if (!maxInclusive && score == maxScore) {
            return false;
        }
        return true;
    }

    std::vector<std::pair<double, std::string>> rangeByScorePairs(const ReferenceState &ref, double minScore,
                                                                  bool minInclusive, double maxScore,
                                                                  bool maxInclusive) {
        std::vector<std::pair<double, std::string>> out;
        if (ref.ordered.empty() || minScore > maxScore || (minScore == maxScore && (!minInclusive || !maxInclusive))) {
            return out;
        }

        auto it = ref.ordered.lower_bound({minScore, ""});
        for (; it != ref.ordered.end(); ++it) {
            if (it->first > maxScore || (!maxInclusive && it->first == maxScore)) {
                break;
            }
            if (inScoreRange(it->first, minScore, minInclusive, maxScore, maxInclusive)) {
                out.push_back(*it);
            }
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

    std::vector<std::pair<double, std::string>> rangeByIndexPairs(const ReferenceState &ref, int l, int r) {
        std::vector<std::pair<double, std::string>> ordered = orderedPairs(ref);
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

    std::vector<std::string> firstScoreSegmentValues(const ReferenceState &ref) {
        std::vector<std::string> out;
        if (ref.ordered.empty()) {
            return out;
        }

        const double lexScore = ref.ordered.begin()->first;
        for (auto it = ref.ordered.begin(); it != ref.ordered.end() && it->first == lexScore; ++it) {
            out.push_back(it->second);
        }
        return out;
    }

    int countByLexReference(const ReferenceState &ref, const ZSkipList::LexBound &min, const ZSkipList::LexBound &max) {
        if (ref.ordered.empty()) {
            return 0;
        }

        if (min.type == ZSkipList::LexBound::Type::POS_INF || max.type == ZSkipList::LexBound::Type::NEG_INF) {
            return 0;
        }

        if (min.type == ZSkipList::LexBound::Type::VALUE && max.type == ZSkipList::LexBound::Type::VALUE) {
            if (min.value > max.value || (min.value == max.value && (!min.inclusive || !max.inclusive))) {
                return 0;
            }
        }

        std::vector<std::string> values = firstScoreSegmentValues(ref);

        auto startIt = values.begin();
        if (min.type == ZSkipList::LexBound::Type::VALUE) {
            startIt = min.inclusive ? std::lower_bound(values.begin(), values.end(), min.value)
                                    : std::upper_bound(values.begin(), values.end(), min.value);
        }

        auto stopIt = values.end();
        if (max.type == ZSkipList::LexBound::Type::VALUE) {
            stopIt = max.inclusive ? std::upper_bound(values.begin(), values.end(), max.value)
                                   : std::lower_bound(values.begin(), values.end(), max.value);
        }

        if (startIt > stopIt) {
            return 0;
        }
        return static_cast<int>(stopIt - startIt);
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
        list.getRangeByScore(std::numeric_limits<double>::lowest(), true, std::numeric_limits<double>::max(), true,
                             actual);
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
                list.getRangeByScore(lo, true, hi, true, actual);
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

    ZSkipList::LexBound randomLexBound(test_utils::RandomSource &random, const std::vector<std::string> &samples) {
        ZSkipList::LexBound bound;

        int typePick = random.randomIndex(0, 9);
        if (typePick == 0) {
            bound.type = ZSkipList::LexBound::Type::NEG_INF;
            return bound;
        }
        if (typePick == 1) {
            bound.type = ZSkipList::LexBound::Type::POS_INF;
            return bound;
        }

        bound.type = ZSkipList::LexBound::Type::VALUE;
        bound.inclusive = random.chance(0.5);

        if (!samples.empty() && random.chance(0.75)) {
            bound.value = samples[random.randomIndex(0, static_cast<int>(samples.size()) - 1)];
        } else {
            bound.value = random.randomString(1, 12);
        }
        return bound;
    }

    void randomLexCountAndRangeSegmentTest(int firstSegmentCount, int otherSegmentCount, int countQueryCount,
                                           int rangeQueryCount, unsigned seed) {
        test_utils::RandomSource random(seed);
        ZSkipList list;
        ReferenceState ref;

        const double firstScore = random.randomScore();
        const double secondScore = firstScore + 1000.0;

        for (int i = 0; i < firstSegmentCount; ++i) {
            std::string value;
            do {
                value = random.randomString(2, 10);
            } while (ref.dict.find(value) != ref.dict.end());

            ref.ordered.insert({firstScore, value});
            ref.dict[value] = firstScore;
            INFO("seed " << seed << " insert first segment " << value);
            REQUIRE(list.insertOrUpdate(firstScore, value));
        }

        for (int i = 0; i < otherSegmentCount; ++i) {
            std::string value;
            do {
                value = random.randomString(2, 10);
            } while (ref.dict.find(value) != ref.dict.end());

            ref.ordered.insert({secondScore, value});
            ref.dict[value] = secondScore;
            INFO("seed " << seed << " insert second segment " << value);
            REQUIRE(list.insertOrUpdate(secondScore, value));
        }

        std::vector<std::string> firstValues = firstScoreSegmentValues(ref);
        REQUIRE(static_cast<int>(firstValues.size()) == firstSegmentCount);

        {
            ZSkipList::LexBound min;
            ZSkipList::LexBound max;
            min.type = ZSkipList::LexBound::Type::NEG_INF;
            max.type = ZSkipList::LexBound::Type::POS_INF;
            INFO("seed " << seed << " full span count");
            REQUIRE(list.countByLex(min, max) == firstSegmentCount);
        }

        {
            ZSkipList::LexBound min;
            ZSkipList::LexBound max;
            min.type = ZSkipList::LexBound::Type::POS_INF;
            max.type = ZSkipList::LexBound::Type::POS_INF;
            INFO("seed " << seed << " pos-inf count");
            REQUIRE(list.countByLex(min, max) == 0);
        }

        for (int q = 0; q < countQueryCount; ++q) {
            ZSkipList::LexBound min = randomLexBound(random, firstValues);
            ZSkipList::LexBound max = randomLexBound(random, firstValues);

            int expected = countByLexReference(ref, min, max);
            int got = list.countByLex(min, max);
            INFO("seed " << seed << " count query " << q);
            INFO("min type " << static_cast<int>(min.type) << " value " << min.value << " incl " << min.inclusive);
            INFO("max type " << static_cast<int>(max.type) << " value " << max.value << " incl " << max.inclusive);
            REQUIRE(got == expected);
        }

        const double lexScore = ref.ordered.begin()->first;
        std::vector<std::string> actual;
        for (int q = 0; q < rangeQueryCount; ++q) {
            std::string a = firstValues[random.randomIndex(0, static_cast<int>(firstValues.size()) - 1)];
            std::string b = random.randomString(2, 20);

            std::string lo = a < b ? a : b;
            std::string hi = a < b ? b : a;

            auto expected = rangeByLex(ref, lo, hi, lexScore);
            list.getRangeByLex(lo, hi, actual);
            INFO("seed " << seed << " lex range query " << q << " [" << lo << ", " << hi << "]");
            REQUIRE(actual == expected);
        }
    }

    void randomGetScoreOpsTest(int operations, unsigned seed) {
        test_utils::RandomSource random(seed);
        ZSkipList list;
        ReferenceState ref;

        for (int i = 1; i <= operations; ++i) {
            int action = random.randomIndex(0, 2); // 0 insert/update, 1 erase, 2 lookup

            if (action == 0) {
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
                INFO("op " << i << " seed " << seed << " insert/update value " << value << " score " << score);
                REQUIRE(got == isNew);
            } else if (action == 1) {
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
            } else {
                bool pickExisting = !ref.dict.empty() && random.chance(0.7);
                std::string value = pickExisting ? pickExistingValue(ref, random) : random.randomString();

                double gotScore = 0.0;
                bool got = list.getScore(value, gotScore);

                auto it = ref.dict.find(value);
                bool expected = it != ref.dict.end();
                INFO("op " << i << " seed " << seed << " lookup value " << value);
                REQUIRE(got == expected);
                if (expected) {
                    REQUIRE(gotScore == it->second);
                }
            }

            REQUIRE(list.size() == static_cast<int>(ref.dict.size()));
            if (i % 50 == 0) {
                verifyFullOrder(list, ref, "getScore ops full order after " + std::to_string(i));
            }
        }
    }

    void randomRangeWithScoresTest(int valueCount, int queryCount, unsigned seed) {
        test_utils::RandomSource random(seed);
        ZSkipList list;
        ReferenceState ref;

        for (int i = 0; i < valueCount; ++i) {
            std::string value;
            do {
                value = random.randomString(2, 10);
            } while (ref.dict.find(value) != ref.dict.end());

            double score = random.randomScore();
            ref.ordered.insert({score, value});
            ref.dict[value] = score;
            INFO("seed " << seed << " insert " << value << " score " << score);
            REQUIRE(list.insertOrUpdate(score, value));
        }

        for (int q = 0; q < queryCount; ++q) {
            if (random.chance(0.5)) {
                int maxIndex = static_cast<int>(ref.dict.size()) + 15;
                int l = random.randomIndex(-5, maxIndex);
                int r = random.randomIndex(-5, maxIndex);
                if (l > r) {
                    std::swap(l, r);
                }

                auto expected = rangeByIndexPairs(ref, l, r);
                std::vector<std::pair<double, std::string>> actual;
                list.getRange(l, r, actual);
                INFO("seed " << seed << " index range query " << q << " [" << l << ", " << r << "]");
                REQUIRE(actual == expected);
            } else {
                double a = random.randomScore();
                double b = random.randomScore();
                double lo = std::min(a, b);
                double hi = std::max(a, b);
                bool minInclusive = random.chance(0.5);
                bool maxInclusive = random.chance(0.5);

                auto expected = rangeByScorePairs(ref, lo, minInclusive, hi, maxInclusive);
                std::vector<std::pair<double, std::string>> actual;
                list.getRangeByScore(lo, minInclusive, hi, maxInclusive, actual);
                INFO("seed " << seed << " score range query " << q << " [" << lo << ", " << hi << "]"
                             << " minInc " << minInclusive << " maxInc " << maxInclusive);
                REQUIRE(actual == expected);
            }
        }
    }

} // namespace

TEST_CASE("skip list randomized mixed score operations", "[zskiplist][score]") {
    const unsigned seed = test_utils::chooseSeed();
    INFO("SKIPLIST_SEED=" << seed);
    randomScoreOpsTest(500, seed);
}

TEST_CASE("skip list lexicographical ranges with equal scores", "[zskiplist][lex]") {
    const unsigned baseSeed = test_utils::chooseSeed();
    const unsigned seed = baseSeed ^ 0x9E3779B9u;
    INFO("SKIPLIST_SEED=" << baseSeed);
    INFO("DERIVED_SEED=" << seed);
    randomLexRangeTest(80, 120, seed);
}

TEST_CASE("skip list randomized lex count and lex range honor first score segment", "[zskiplist][lexcount][segment]") {
    const unsigned baseSeed = test_utils::chooseSeed();
    const unsigned seed = baseSeed ^ 0xA5A5A5A5u;
    INFO("SKIPLIST_SEED=" << baseSeed);
    INFO("DERIVED_SEED=" << seed);
    randomLexCountAndRangeSegmentTest(60, 60, 300, 200, seed);
}

TEST_CASE("skip list getScore randomized lookups", "[zskiplist][score][lookup]") {
    const unsigned baseSeed = test_utils::chooseSeed();
    const unsigned seed = baseSeed ^ 0xC0FFEEu;
    INFO("SKIPLIST_SEED=" << baseSeed);
    INFO("DERIVED_SEED=" << seed);
    randomGetScoreOpsTest(350, seed);
}

TEST_CASE("skip list range overload returns (score, member) pairs", "[zskiplist][range][scores]") {
    const unsigned baseSeed = test_utils::chooseSeed();
    const unsigned seed = baseSeed ^ 0x9E3779B9u;
    INFO("SKIPLIST_SEED=" << baseSeed);
    INFO("DERIVED_SEED=" << seed);
    randomRangeWithScoresTest(140, 300, seed);
}
