/**
 * @file proxy.h
 * @brief 定义协议结构体，为服务器和客户端所共用。
 */

#ifndef PROXY_H

#define PROXY_H

/* 单个用户与服务器交互（注册、登录、退出）*/

//客户端报文类型
#define REGISTER 0x01  //注册
#define LOGIN 0x02     //登录
#define LOGOUT 0x03    //退出

/*#pragma(1)
struct ClientSingle {
	uint8_t type;     //报文类型
	char userID[10];  //用户ID
	char passwd[10];  //用户密码
};
#pragma()*/

//服务器报文类型
#define REGISTER_ACK 0x01
#define ID_CONFICT 0x02  //用户ID冲突
#define LOGIN_ACK 0x03
#define LOGOUT_ACK 0x04

/*#pragma(1)
struct ServerSingle {
	uint8_t type;    //报文类型
	uint8_t num;     //用户个数（注册，退出返回0x01;相应登录的报文时返回当前在线玩家的个数）
	char data[200];  //依次是各个用户ID和用户状态，最多可以包含10个用户信息
};
#pragma()*/

/* 对战报文 */
#define ASK_BATTLE 0x10   //发起对战请求
#define YES_BATTLE 0x20   //接受对战
#define NO_BATTLE 0x30    //拒绝对战
#define IN_BATTLE 0x40    //正在对战报文
#define END_BATTLE 0x50   //某一方血量为0，对战结束

//对战招数
#define STONE 0x01     //石头
#define SCISSOR 0x02   //剪刀
#define PAPER 0x03     //布

/*#pragma(1)
struct ClientBattle {
	uint8_t type;    //报文类型
	char srcID[10];  //发起对战请求的用户ID
	char dstID[10];  //被邀对战的用户ID
	uint8_t attack;  //发出招数
};
#pragma()

#pragma(1)
struct ServerBattle {
	uint8_t type;   //报文类型
	char srcID[10]; //双方ID
	char dstID[10];
	uint8_t src_attack;  //双方出招
	uint8_t dst_attack;
	uint8_t srcHP;  //双方血量
	uint8_t dstHP;
};
#pragma()*/

//合并了两种类型的报文，具体的报文类型由type字段决定，服务器端类似
#pragma(1)
struct client {
	uint8_t type;
	union {
		struct {
			char userID[10];
			char passwd[10];
		} single;
		struct {
			char srcID[10];
			char dstID[10];
			uint8_t attack;
		} battle;
	};
};
#pragma()

#pragma(1)
struct server {
	uint8_t type;
	union {
		struct {
			uint8_t num;
			char data[200];
		} single;
		struct {
			char srcID[10];
			char dstID[10];
			uint8_t srcattack;
			uint8_t dstattack;
			uint8_t srcHP;
			uint8_t dstHP;
		} battle;
	};
};
#pragma()
