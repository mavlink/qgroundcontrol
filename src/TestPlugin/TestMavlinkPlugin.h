// //
// // Created by Administrator_lt on 25-6-27.
// //
// #pragma once
//
// #include <QObject>
//
// #include "mavlink.h"
// #include "LinkInterface.h"
//
//
// //该类的作用，作为一个 MAVLink 通信测试插件，能主动向飞控发送 MAVLink 消息，也能被动接收飞控发回的消息，并打印或处理。
// class TestMavlinkPlugin : public QObject {
//     // Q_OBJECT QT的宏用于启用信号和槽（singal slots）机制
//     Q_OBJECT
//
//    public:
//     // 构造函数的声明 关键字explicit防止隐式类型转换
//     // QObject* parent = nullptr默认参数指向QT的父对象，用于内存管理和对象树结构
//     explicit TestMavlinkPlugin(QObject *parent = nullptr);
//     void sendHeartbeat();  // 主动发送心跳
//
//     // Qt槽函数 所有写在这里的函数都可以被connect信号连接机制自动调用
//    private slots:
//     // link 表示消息是通过哪个串口/udp 链路接收数据
//     // MAVlink协议中的完整消息结构
//     void _mavlinkMessageReceived(LinkInterface *link, mavlink_message_t message);  // 被动接收消息
//    private:
//     // 私有函数封装如何向当前连接到Vehicle飞控发送一条MAVlink 消息
//     void _sendMessage(const mavlink_message_t &msg);
// };

