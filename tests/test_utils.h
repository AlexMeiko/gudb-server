#pragma once

#include <cstdlib>
#include <random>
#include <string>

namespace test_utils {

    /**
     * @brief 测试随机工具，内部持有 rng 状态
     */
    struct RandomSource {
        explicit RandomSource(unsigned seed) : rng(seed) {}

        /**
         * @brief 生成 [-100, 100] 区间的两位小数分数
         * @return 随机分数
         */
        double randomScore() {
            std::uniform_int_distribution<int> dist(-10000, 10000);
            return dist(rng) / 100.0;
        }

        /**
         * @brief 生成随机长度的字母数字字符串
         * @param minLen 最小长度
         * @param maxLen 最大长度
         * @return 随机字符串
         */
        std::string randomString(int minLen = 3, int maxLen = 14) {
            static const std::string chars = "abcdefghijklmnopqrstuvwxyz"
                                             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                             "0123456789";
            std::uniform_int_distribution<int> lenDist(minLen, maxLen);
            std::uniform_int_distribution<int> charDist(0, static_cast<int>(chars.size()) - 1);
            int len = lenDist(rng);
            std::string out(len, '\0');
            for (int i = 0; i < len; ++i) {
                out[i] = chars[charDist(rng)];
            }
            return out;
        }

        /**
         * @brief 返回 [low, high] 的均匀随机整数
         * @param low 下界
         * @param high 上界
         * @return 随机整数
         */
        int randomIndex(int low, int high) {
            std::uniform_int_distribution<int> dist(low, high);
            return dist(rng);
        }

        /**
         * @brief 概率为 probability 的伯努利试验
         * @param probability 返回 true 的概率
         * @return true / false
         */
        bool chance(double probability) {
            std::bernoulli_distribution dist(probability);
            return dist(rng);
        }

        std::mt19937 rng;
    };

    /**
     * @brief 若设置了环境变量 SKIPLIST_SEED 则使用之，否则使用随机熵源
     * @return 随机种子
     */
    inline unsigned chooseSeed() {
        if (const char *env = std::getenv("SKIPLIST_SEED")) {
            char *end = nullptr;
            unsigned long parsed = std::strtoul(env, &end, 10);
            if (end && *end == '\0') {
                return static_cast<unsigned>(parsed);
            }
        }
        return std::random_device{}();
    }

} // namespace test_utils
