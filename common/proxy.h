/**
 * @file proxy.h
 * @brief 定义协议结构体，为服务器和客户端所共用。
 */

#ifndef PROXY_H
#define PROXY_H

#include <inttypes.h>

/* 单个用户与服务器交互（注册、登录、退出）*/

// 方便为没有名字的子子结构体构造指针
#define MAKE_PTR(p, type) typeof(&type) p = &type

//客户端报文类型
#define REGISTER 0x01  //注册
#define LOGIN 0x02     //登录
#define LOGOUT 0x03    //退出

//服务器报文类型
#define REGISTER_ACK 0x01
#define ID_CONFLICT 0x02  //用户ID冲突
#define LOGIN_ACK 0x03
#define LOGIN_ERROR 0x04 //用户ID和密码不匹配
#define LOGOUT_ACK 0x05
#define LOGIN_ANNOUNCE 0x06 //通知其他用户有用户上线
#define LOGOUT_ANNOUNCE 0x07 //通知其他用户有用户下线
#define BATTLE_ANNOUNCE 0x08 //通知其他用户有用户进入游戏状态

#define LIST_UPDATE 0x05  //更新列表

//对战报文类型
#define ASK_BATTLE 0x10   //发起对战请求
#define YES_BATTLE 0x20   //接受对战
#define NO_BATTLE 0x30    //拒绝对战
#define IN_BATTLE 0x40    //正在对战报文
#define END_BATTLE 0x50   //某一方血量为0，对战结束

//对战招数
#define STONE 0x01     //石头
#define SCISSOR 0x02   //剪刀
#define PAPER 0x03     //布

//用户状态
#define ONLINE 0x01    //在线
#define OFFLINE 0x02   //离线
#define PLAY 0x03      //正在对战

//合并了两种类型的报文，具体的报文类型由type字段决定，服务器端类似
#pragma pack(1)
typedef struct {
	uint8_t type;
	union { 
		struct {
			char userID[10];
			char passwd[10];
		} account;
		struct {
			char srcID[10];
			char dstID[10];
			uint8_t attack;
		} battle;
	};
} Request;
#pragma pack()

#pragma pack(1)
typedef struct {
	uint8_t type;
	union {
		struct {
			int num;
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
        // 列表创建报文，nr_entry 指示之后要接收多少个 PlayerEntry 结构体
        struct {
            int nr_entry;
        } list;
        // 列表更新报文，表明列表变化内容
        struct {
#define LIST_ADD 1
#define LIST_DEL 2
#define LIST_RANK 3
#define LIST_FRIEND 4
            int change;
            char userID[10];
            int new_rank;
        } list_chg;
	};
} Response;
#pragma pack()

#pragma pack(1)
/**
 * @brief 玩家列表条目的结构
 */
typedef struct {
	char userID[10];  // 用户名
	int rank;         // 排名
	int state;        // 状态（对战/空闲）
    int is_friend;    // 与当前请求的玩家是否是好友
} PlayerEntry;
#pragma pack()

#endif
