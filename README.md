# Cluster-Chat-Server

在 Linux 环境下基于 muduo 开发的集群聊天服务器。实现新用户注册、用户登录、添加好友、添加群组、好友通信、群组聊天、保持离线消息等功能。

## 项目特点

- 基于 muduo 网络库开发网络核心模块，实现高效通信
- 使用第三方 JSON 库实现通信数据的序列化和反序列化
- 使用 Nginx 的 TCP 负载均衡功能，将客户端请求分派到多个服务器上，以提高并发处理能力
- 基于发布-订阅的服务器中间件redis消息队列，解决跨服务器通信难题
- 封装 MySQL 接口，将用户数据储存到磁盘中，实现数据持久化
- 基于 CMake 构建项目

## 必要环境

- 安装`boost`库
- 安装`muduo`库
- 安装`Nginx`
- 安装`redis`

## 构建项目

创建数据库

```shell
# 连接MySQL
mysql -u root -p your passward
# 创建数据库
create database chat;
# 执行数据库脚本创建表
source chat.sql
```

执行脚本构建项目

```shell
bash build.sh
```

## 执行生成文件

```shell
# 启动服务端
cd ./bin
./ChatServer 6000 
```

```shell
# 启动客户端
./ChatClient 127.0.0.1 8000
```
