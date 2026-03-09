# gudb-server

轻量级 Redis 风格的内存键值数据库（C++20 实现），兼容 RESP 与基础 redis-cli 命令。

- **支持类型**：String、List、Set、Hash、ZSet（基础命令已实现）
- **协议**：Redis RESP（支持复用 redis-cli 交互）
- **网络**：基于 epoll 的非阻塞 IO（仅 Linux）
- **构建工具**：CMake

---

## 🚀 快速开始

### 环境要求

- **操作系统**: Linux (推荐 Ubuntu/Debian/CentOS)
  > 注意：由于使用了 `epoll`，本项目目前仅支持 Linux 环境。
- **编译器**: 支持 C++20 的编译器 (GCC 10+, Clang 10+)
- **构建工具**: CMake >= 3.12.0

### 🛠️ 构建与安装

推荐使用 out-of-source 构建：

```bash
# 1) 生成构建目录（默认 Release）
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 2) 开始编译
cmake --build build -- -j$(nproc)
```

> 调试模式：`cmake -B build -DCMAKE_BUILD_TYPE=Debug`  
> 静态编译：`cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_STATIC=ON`（可能需要对应的静态库）

#### 运行服务

```bash
./build/src/gudb-server
# 默认监听端口: 6378
```

#### 安装（可选）

```bash
sudo cmake --install build
```

安装后可直接在终端运行 `gudb-server`。

---

## 🧪 测试

项目附带 ZSkipList 的单元测试，依赖 Catch2：

```bash
# 需要先安装 Catch2（例如 Debian/Ubuntu: sudo apt-get install catch2）
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target test_gudb
ctest --test-dir build
```

未安装 Catch2 时会跳过测试构建。

> 说明：测试模块大部分代码暂由 AI 编写维护，目前专注主线开发

---

## 💻 使用说明

推荐直接使用标准的 `redis-cli` 进行连接：

```bash
redis-cli -p 6378
```

交互示例：

```bash
127.0.0.1:6378> SET mykey "Hello World"
OK
127.0.0.1:6378> GET mykey
"Hello World"
127.0.0.1:6378> TTL mykey
(integer) -1
```

也可以使用`telnet` / `nc` 进行连接测试。

---

## 📊 性能测试（Benchmark）

以下结果基于同一台 Linux 主机的 `redis-benchmark` 压测：

- `6378`：`gudb-server`
- `6379`：官方 `Redis`
- 工具：`redis-benchmark`
- 连接保持开启：`-k 1`
- 压测参数：非 pipeline 使用 `-c 10 -n 500000`，pipeline 使用 `-c 10 -n 10000000 -P 500`

> 说明：压测过程中 `gudb-server` 输出 `WARNING: Could not fetch server CONFIG`，这是因为当前未实现 `CONFIG` 命令，不影响 `redis-benchmark` 对目标命令的实际压测。

### 压测命令

非 pipeline 场景：

```bash
redis-benchmark -p 6379 -t set,get,hset,zadd,sadd -c 10 -n 500000 -q -k 1
redis-benchmark -p 6378 -t set,get,hset,zadd,sadd -c 10 -n 500000 -q -k 1
```

pipeline 场景：

```bash
redis-benchmark -p 6379 -t set,get,hset,zadd,sadd -c 10 -n 10000000 -P 500 -q -k 1
redis-benchmark -p 6378 -t set,get,hset,zadd,sadd -c 10 -n 10000000 -P 500 -q -k 1
```

### 非 pipeline：与 Redis 基本持平

| Command | Redis QPS | gudb QPS | 对比 |
| --- | ---: | ---: | ---: |
| SET  | 65,385 | 65,445 | +0.1% |
| GET  | 66,756 | 67,060 | +0.5% |
| SADD | 66,472 | 67,250 | +1.2% |
| HSET | 66,216 | 66,551 | +0.5% |
| ZADD | 66,587 | 67,531 | +1.4% |

- 在 `-c 10`、无 pipeline 的条件下，`gudb-server` 与 Redis 吞吐处于同一量级。
- 两者 `p50` 延迟均约为 `0.071 ms`。
- 从结果来看，当前瓶颈可能不主要来自服务端实现

### 深 pipeline：

| Command | Redis QPS | gudb QPS | 对比 |
| --- | ---: | ---: | ---: |
| SET  | 1,889,288 | 2,901,915 | +53.6% |
| GET  | 2,515,091 | 2,390,629 | -4.9% |
| SADD | 2,158,429 | 2,306,273 | +6.8% |
| HSET | 1,681,237 | 1,991,635 | +18.5% |
| ZADD | 1,575,548 | 1,883,239 | +19.5% |

- 深 pipeline 的收益主要来自批处理显著摊薄了 RTT、系统调用和事件循环调度开销，更聚焦于吞吐能力的测试。

### 测试环境说明

- 测试环境为某云 `1 vCPU / 1 GiB` Linux 实例。由于`redis-benchmark` 与服务端运行在同一台机器上，压测进程与服务端共享 CPU / 内存资源，存在一定资源竞争。
- 因此，上述结果不应被视为独立压测环境下的绝对性能上限。

## ⚙️ 支持命令

- 基础：`PING`, `ECHO`, `TIME`, `FLUSHDB`
- 字符串 (String)：`SET`, `GET`, `GETRANGE`, `GETSET`, `MGET`, `MSET`, `SETNX`, `MSETNX`, `STRLEN`, `APPEND`, `SETEX`,
  `PSETEX`, `INCR`, `INCRBY`, `DECR`, `DECRBY`
- 键 (Key)：`DEL`, `EXISTS`, `EXPIRE`, `EXPIREAT`, `PEXPIRE`, `PEXPIREAT`, `PERSIST`, `TTL`, `PTTL`, `RENAME`,
  `RENAMENX`, `TYPE`
- 列表 (List)：`LPUSH`, `LPOP`
- 集合 (Set)：`SADD`, `SMEMBERS`
- 哈希 (Hash)：`HSET`, `HGET`, `HINCRBY`
- 有序集合 (ZSet)：`ZADD`, `ZCARD`, `ZCOUNT`, `ZINCRBY`, `ZINTERSTORE`, `ZLEXCOUNT`, `ZRANGE`, `ZRANGEBYSCORE`

> ⚠️ 命令兼容性说明：
> - 未列出的 Redis 命令暂不支持。
> - 部分命令仅实现基础语法/核心功能，暂不支持以下参数/选项：
    >
- `ZINTERSTORE`：暂不支持 `WEIGHTS` / `AGGREGATE`（聚合方式固定为 score 求和）。
>   - `ZADD`：暂不支持 `NX` / `XX` / `CH` / `INCR`。
>   - `ZRANGE`：暂不支持 `REV` / `BYSCORE` / `BYLEX` / `LIMIT`。
>   - `ZRANGEBYSCORE`：暂不支持 `LIMIT`。
>   - `HSET`：暂不支持一次写入多个字段值对。
>   - `LPOP`：暂不支持 `LPOP key count`。
>   - `EXPIRE` / `PEXPIRE` / `EXPIREAT` / `PEXPIREAT`：暂不支持 `NX` / `XX` / `GT` / `LT`。

---

## 🗂 项目结构

```text
src/
├── CMakeLists.txt
├── main.cpp                # 程序入口
├── core/                   # 基础设施 (Buffer, Logger 等)
├── protocol/               # RESP 协议编解码 (Parser, Encoder)
├── ds/                     # 数据结构 (ZSkipList 等)
├── db/                     # 数据库核心逻辑 (Database, Object)
├── cmd/                    # 命令实现 (自动注册机制)
└── net/                    # 网络层 (Server, Connection, Epoll)
tests/                      # 单元测试
```

### 核心模块

- `db/Object.h`：统一的数据对象封装，使用 `std::variant` 持有不同类型的值。
- `db/Database.h`：内存数据库核心实现，包含键空间存储与过期管理。
- `ds/ZSkipList.h`：有序集合的跳表实现，支持按 score/lex 范围查询。
- `protocol/Parser.h`：RESP 协议解析器，处理客户端请求。
- `cmd/Registry.h`：命令注册中心，利用静态全局变量实现命令的自动注册。
- `net/Server.cpp`：基于 `epoll` 的事件循环与连接管理。

---

## 🧭 开发者指南

### 添加新命令

在 `src/cmd/` 目录下新增 `.cpp` 文件（如 `hello.cpp`），并利用 `AutoRegister` 完成注册：

```cpp
#include "Registry.h"
#include "../protocol/Encoder.h"

namespace gudb::cmd {
    // 命令实现函数
    std::string helloCommand(const std::vector<std::string> &args, Database &db) {
        return protocol::Encoder::encodeSimpleString("Hello World");
    }

    // 注册命令 (命令名不区分大小写，内部统一转大写)
    static AutoRegister reg_hello("HELLO", helloCommand);
}
```

### 函数签名与规范

命令函数签名目前是 `std::string func(std::vector<std::string> &args, Database &db)`（某些命令使用非 `const` 引用并会修改
`args`），但推荐使用 `const std::vector<std::string>&` 来避免副作用：

- 若函数需要修改 `args`（极少数场景），请明确文档说明。

### 过期策略

- 采用惰性删除：`db.get()` / `db.exists()` 等在访问时检查过期并清理。
- 过期时间以毫秒时间戳存储，`expiresAt = -1` 表示永不过期。

### 性能提示

- 可以在适当场景使用 `swap` 避免拷贝（若修改了来源数据，请在注释中说明）。

---

## 📌 待办事项 / 已知问题

- [x] **数据结构**: 完善跳表 (SkipList) 。
- [ ] **命令补充**：`LRANGE`、`LREM`、`SREM` 以及更多 ZSet 命令（`ZREM`、`ZSCORE`、`ZRANGEBYLEX` 等）。
- [ ] **持久化**：支持 RDB/AOF 等落盘方案。
- [ ] **配置增强**：支持命令行参数（如 `-p <port>`）与配置文件。
- [ ] **多系统支持**：当前依赖 Linux `epoll`，暂不支持 Windows/macOS 原生构建。

---

## 📏 开发规范

以下是目前项目所使用的规范，如果更好的提议，请务必让我知道:

### 文件命名

- 类文件：使用 PascalCase（大驼峰），文件名与类名保持一致，例如 `Command.h`, `Registry.cpp`。
- 非类文件/功能模块：使用 snake_case（全小写），适用于包含一组函数或无特定类的实现文件，例如 `string.cpp`, `utils.h`,
  `main.cpp`。

如有更好的建议，欢迎提出。

---

## 🤝 贡献

1. Fork 仓库
2. 创建分支：`feature/your-feature` 或 `fix/issue-number`
3. 编写代码并补充或更新测试
4. 提交 PR，并描述变更内容与理由

---

## 📜 许可

本项目遵循 Apache 2.0 许可证（详见根目录 `LICENSE` 文件）。
