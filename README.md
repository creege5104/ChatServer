# ChatServer
集群聊天服务器与客户端，基于 nginx 与 muduo 网络库，数据库包括 MySQL 与 Redis

## 编译方式
cd build  
rm -rf *  
cmake..  
make  

## 项目简介

**项目名称**：集群聊天服务器（ChatServer）  
**项目地址**：[https://github.com/creege5104/ChatServer](https://github.com/creege5104/ChatServer)  
**开发环境**：VSCode 远程 Linux 开发，CMake 构建，Shell 自动化编译脚本  
**技术栈**：C++17、muduo、MySQL、Redis、JSON、nginx  

**项目内容**：
1. 以 muduo 网络库作为网络核心模块，基于 Reactor 模型提供高并发网络 IO 服务，实现网络层与业务层解耦；
2. 设计基于 JSON 序列化/反序列化的私有通信协议，支持用户登录/注册、一对一聊天、群聊、好友与群组管理、离线消息等核心功能；
3. 可配合 nginx TCP 负载均衡实现服务器集群部署，提升后端并发处理能力；
4. 基于 Redis 发布-订阅机制实现跨服务器消息路由，完成多节点间的用户消息投递；
5. 使用 MySQL 关系型数据库持久化存储用户、好友、群组及离线消息数据。

**个人收获**：
- 熟悉基于开源网络库的服务端程序设计，掌握高并发网络 IO 与业务逻辑分离的架构思想；
- 掌握 nginx TCP 负载均衡配置，以及 Redis、MySQL 等存储与中间件在服务端项目中的实际应用；
- 理解集群场景下跨节点消息路由与离线消息投递的设计与实现。

**问题与解决方案**：
通过自定义脚本或 JMeter 对服务器进行并发压测，结合调整进程文件描述符（fd）上限等系统参数，提升服务器的并发承载能力。