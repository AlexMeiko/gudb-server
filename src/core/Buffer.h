#pragma once
#include <string>
#include <sys/types.h>
#include <vector>

namespace gudb::core {
    class Buffer {
    public:
        /**
         * @brief 向缓冲区追加数据
         *
         * 将指定长度的数据追加到缓冲区中。如果缓冲区空间不足，会自动扩容。
         * 数据会被写入到当前写指针位置，然后写指针向前移动相应长度。
         *
         * @param data 要追加的数据指针
         * @param len 要追加的数据长度
         */
        void append(const char *data, size_t len);

        /**
         * @brief 查看当前可读数据
         *
         * 返回指向当前读指针位置的常量指针，用于查看但不移除数据。
         * 这个指针指向缓冲区中下一个要读取的数据位置。
         *
         * @return const char* 指向可读数据的指针
         */
        const char *peek() const;

        /**
         * @brief 移除已读取的数据
         *
         * 从缓冲区中移除指定长度的数据，相当于移动读指针。
         * 如果移除的长度大于等于可读字节数，则重置缓冲区（读指针和写指针都归零）。
         * 否则，只移动读指针向前指定长度。
         *
         * @param len 要移除的数据长度
         */
        void retrieve(size_t len);

        /**
         * @brief 获取可读字节数
         *
         * 计算缓冲区中尚未被读取的数据字节数。
         * 可读字节数 = 写指针位置 - 读指针位置
         *
         * @return size_t 可读字节数
         */
        size_t readableBytes() const;

        size_t writableBytes() const;

        char *beginWrite();

        ssize_t readFd(int fd, int *savedErrno);

    private:
        std::vector<char> buf_;
        size_t readIndex_ = 0;
        size_t writeIndex_ = 0;
    };
} // namespace gudb::core
