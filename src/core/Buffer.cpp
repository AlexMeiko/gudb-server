#include "Buffer.h"
#include <algorithm>
#include <iterator>

namespace gudb::core {
    // 向缓冲区追加数据
    void Buffer::append(const char *data, size_t len) {
        // 检查当前缓冲区容量是否足够，如果不够则扩容
        if (writeIndex_ + len > buf_.size()) {
            buf_.resize(writeIndex_ + len);
        }

        // 使用std::copy将数据复制到缓冲区写指针位置
        std::copy_n(data, len,
                    buf_.begin() + static_cast<std::ptrdiff_t>(writeIndex_));
        writeIndex_ += len;
    }

    // 获取可读字节数
    size_t Buffer::readableBytes() const { return writeIndex_ - readIndex_; }

    // 查看当前可读数据
    const char *Buffer::peek() const { return buf_.data() + readIndex_; }

    // 移除已读取的数据
    void Buffer::retrieve(size_t len) {
        // 如果要移除的长度大于等于可读字节数，重置缓冲区
        if (len >= readableBytes()) {
            readIndex_ = writeIndex_ = 0;
        } else {
            // 否则只移动读指针
            readIndex_ += len;
        }
    }
} // namespace gudb::core
