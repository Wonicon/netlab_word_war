# 网络协议开发实验：文字对战游戏

目前只有串行客户端与并发服务器的基本通信框架。

需要安装库:

```
$ sudo apt-get install libsqlite3-dev
$ sudo apt-get install libncurses5-dev
```

项目结构：

```
.
├── client       // 客户端实现
│   ├── include  //   客户端头文件
│   └── src      //   客户端 C 代码
├── common       // 通用内容，如协议定义
├── Makefile
└── server       // 服务器实现
    ├── include  //   服务器头文件
    └── src      //   服务器 C 代码
```

在项目根目录下执行 `make` 在项目根目录下生成客户端 `client.bin` 和 `server.bin`。

当前用例：

```
# Terminal 1
$ ./server.bin 4000

# Terminal 2
$ ./client.bin 127.0.0.1 4000
```
