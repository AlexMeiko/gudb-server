# gudb-server

轻量级 Redis 风格的内存键值数据库（C++ 实现）。

- **支持类型**：String、List、Set、Hash、ZSet（部分实现）
- **协议**：Redis RESP（兼容 redis-cli 基本命令交互）
- **网络**：基于 epoll 的非阻塞 IO 实现（Linux）
- **构建工具**：CMake（C++20）, 支持 `make` / Ninja

---

## 🚀 快速开始

### 环境要求

- **操作系统**: Linux (推荐 Ubuntu/Debian/CentOS)
  > 注意：由于使用了 `epoll`，本项目目前仅支持 Linux 环境。
- **编译器**: 支持 C++20 的编译器 (GCC 10+, Clang 10+)
- **构建工具**: CMake >= 3.12.0

### 🛠️ 构建与安装

推荐使用 out-of-source 构建方式。

#### 1. 编译项目

```bash
# 创建并配置构建目录 (默认 Release)
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 开始编译
cmake --build build -- -j$(nproc)
```

> **调试模式**: 如需调试，可将配置命令改为 `cmake -B build -DCMAKE_BUILD_TYPE=Debug`。

> **静态编译**: 需要静态链接可执行文件，可启用 `ENABLE_STATIC` 选项：
> ```bash
> cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_STATIC=ON
> ```
> 可能需要安装对应的静态库。

#### 2. 运行服务器

编译完成后，可执行文件位于 `build/src/` 目录下：

```bash
./build/src/gudb-server
# 默认监听端口: 6378
```

#### 3. 安装 (可选)

将 `gudb-server` 安装到系统路径（默认 `/usr/local/bin`）：

```bash
sudo cmake --install build
```

安装完成后，可以直接在终端运行 `gudb-server`。

---

## 💻 使用说明

你可以使用标准的 `redis-cli` 或 `telnet` / `nc` 进行连接测试。

### 使用 redis-cli (推荐)

```bash
# 连接到 6378 端口
redis-cli -p 6378
```

示例交互：

```bash
127.0.0.1:6378> SET mykey "Hello World"
OK
127.0.0.1:6378> GET mykey
"Hello World"
127.0.0.1:6378> TTL mykey
(integer) -1
```

---

## 🔧 支持命令

目前已实现的命令如下：

- **基础命令**: `PING`, `ECHO`, `TIME`, `FLUSHDB`
- **字符串 (String)**: `SET`, `GET`, `GETRANGE`, `GETSET`, `MGET`, `MSET`, `SETNX`, `MSETNX`, `STRLEN`, `APPEND`,
  `SETEX`, `PSETEX`, `INCR`, `INCRBY`, `DECR`, `DECRBY`
- **键操作 (Key)**: `DEL`, `EXISTS`, `EXPIRE`, `EXPIREAT`, `PEXPIRE`, `PEXPIREAT`, `PERSIST`, `TTL`, `PTTL`, `RENAME`,
  `RENAMENX`, `TYPE`
- **列表 (List)**: `LPUSH`, `LPOP`
- **集合 (Set)**: `SADD`, `SMEMBERS`
- **哈希 (Hash)**: `HSET`, `HGET`, `HINCRBY`

> ⚠️ **注意**: 部分 Redis 指令（如 `KEYS`, `RANDOMKEY`, `SETRANGE` 等）尚未完全实现或仅作为占位符。

---

## 📦 项目结构

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

- **db/Object.h**: 统一的数据对象封装，使用 `std::variant` 存储不同类型的值。
- **db/Database.h**: 核心数据库实现，包含 `std::unordered_map` 存储与过期键管理。
- **protocol/Parser.h**: RESP 协议解析器，处理客户端请求。
- **cmd/Registry.h**: 命令注册中心，利用静态全局变量实现命令的自动注册。
- **net/Server.cpp**: 基于 `epoll` 的事件循环与连接管理。

---

## 🧭 开发者指南

### 添加新命令

在 `src/cmd/` 目录下新建 `.cpp` 文件（例如 `hello.cpp`），并利用 `AutoRegister` 进行注册：

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

- 采用惰性删除：`db.get()` 和 `db.exists()` 会检查键是否过期并在需要时删除。
- 存储时间为毫秒级时间戳（`expiresAt`, -1 表示永不过期）。

### 性能提示

- 使用 `swap` 可以在 O(1) 时间复杂度下替换字符串值，若使用了请注明。**注意：未来可能因其修改源数据的副作用而弃用。**

---

## 📌 待办事项 / 已知问题

- [ ] **命令完善**: 补充 `ZADD`, `LREM`, `SREM` 等常用命令。
- [ ] **数据结构**: 完善跳表 (SkipList) 等底层数据结构。
- [ ] **持久化**: 支持数据持久化到硬盘 (RDB/AOF)。
- [ ] **配置增强**: 支持命令行参数 (如 `-p <port>`) 及配置文件。
- [ ] **多系统支持**: 目前强依赖 Linux `epoll`，暂不支持 Windows 和 macOS 原生构建。

---

## 📏 开发规范

以下是目前项目所使用的规范，如果更好的提议，请务必让我知道:

### 文件命名

- **类文件**：使用 **PascalCase**（大驼峰）。文件名应与类名完全一致。
    - 例：`Command.h`, `Registry.cpp`
- **非类文件/功能模块**：使用 **snake_case**（全小写）。适用于包含一组函数或无特定类的实现文件。
    - 例：`string.cpp`, `utils.h`, `main.cpp`

---

## 🤝 贡献

欢迎提交 PR、提出 issue 或贡献代码！请遵循以下流程：

1. Fork 仓库
2. 创建分支：`feature/your-feature` 或 `fix/issue-number`
3. 编写代码并补充或更新测试
4. 提交 PR 并描述变更内容与理由

---

## 📜 许可证

本项目遵循 Apache 2.0 许可证（或参考根目录 LICENSE 文件）。
