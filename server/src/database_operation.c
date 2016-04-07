#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

#define TABLE_NAME "player.db"

//仅在用户登录时使用，记录当前所有用户的id和在线信息
struct online_info {
	int num;
	char data[200];
};

//注册时调用，检查是否有ID相同的用户
static int register_callback(void *num,int argc, char **argv, char **azColName) {
	//num = (int *)num;
	*((int *)num) += 1;
	return 0;
}

//登录时调用，检查用户名和密码是否匹配
static int check_callback(void *num, int argc, char **argv, char **azColName) {
	*((int *)num) += 1;
	return 0;
}

//填充信息时调用，填充所有在线用户的ID和状态
static int login_callback(void *q,int argc,char **argv,char **azColName) {
	int num = ((struct online_info*)q)->num;
	((struct online_info *)q)->num += 1;

	strncpy(((struct online_info *)q)->data + 10 * num,argv[0],9); //填充用户ID;
	if(strcmp(argv[2],"1") == 0)
		strncpy(((struct online_info *)q)->data + 10 * num + 9,"1",1); //填充用户在线信息;
	else if(strcmp(argv[2],"2") == 0)
		strncpy(((struct online_info *)q)->data + 10 *num + 9,"2",1);

	return 0;
}

static int callback(void *NotUsed, int argc, char **argv, char **azColName) {
	int i;
	for(i = 0; i < argc; i++) 
		printf("%s = %s\n",azColName[i],argv[i] ? argv[i] : "NULL");

	printf("\n");
	return 0;
}

/* 初始化一个数据库表 */
int init_table () { 
	sqlite3 *db;
	char *zErrMsg = 0;
	int rc;
	char *sql;

	/*Open database*/
	rc = sqlite3_open(TABLE_NAME,&db);
	if(rc) {
		fprintf(stderr,"Can't open database in init_table: %s\n",sqlite3_errmsg(db));
		exit(0);
	}
	else
		fprintf(stdout,"Opened database successfully in init_table!\n");

	/* Create SQL statement */
    /* TODO 在已有数据库的情况下重复创建 table 会报错 */
	sql = "CREATE TABLE PLAYER("
		  "ID CHAR(20) NOT NULL,"
		  "PASSWD CHAR(20) NOT NULL,"
		  "STATE  INT NOT NULL);";

	/* Execute SQL statement */
	rc = sqlite3_exec(db,sql,callback,0,&zErrMsg);
	if(rc != SQLITE_OK) {
		fprintf(stderr,"SQL error in init_table: %s\n",zErrMsg);
		sqlite3_free(zErrMsg);
	}
	else
		fprintf(stdout,"Table created successfully in init_table\n");

	sqlite3_close(db);
	return 0;
}

/* 用户注册时，在数据库中添加该用户信息 */
int insert_table(char *userID,char *passwd) {
	sqlite3 *db;
	char *zErrMsg = 0;
	int rc;
	char sql[1024];

	/*Open database*/
	rc = sqlite3_open(TABLE_NAME,&db);
	if(rc)	{
		fprintf(stderr,"Can't open database in insert_table: %s\n",sqlite3_errmsg(db));
		exit(0);
	} else
		fprintf(stdout,"Opened database successfully in insert_table!\n");

	/* check if has same UserID */
	sprintf(sql,"SELECT * FROM PLAYER WHERE ID GLOB '%s';",userID);
	
	int num = 0;
	rc = sqlite3_exec(db,sql,register_callback,&num,&zErrMsg);
	if(rc != SQLITE_OK) {
		fprintf(stderr,"SQL error in insert_table: %s\n",zErrMsg);
		sqlite3_free(zErrMsg);
	} else 
		fprintf(stdout, "Check operation done successfully in insert_table\n");

	fprintf(stdout,"In insert_table: num = %d\n",num);
	//没有相同的ID
	if(num == 0) {
		memset(sql,0,1024);
		sprintf(sql,"INSERT INTO PLAYER (ID,PASSWD,STATE) VALUES('%s','%s',0)",userID,passwd);

		rc = sqlite3_exec(db,sql,callback,0,&zErrMsg);
		if(rc != SQLITE_OK) {
			fprintf(stderr,"SQL error in insert_table: %s\n",zErrMsg);
			sqlite3_free(zErrMsg);
			sqlite3_close(db);
			return -1;
		} else {
			fprintf(stdout,"Insert done successfully in insert_table\n");
			sqlite3_close(db);
			return 0;
		}

	}
	//有相同的ID
	else {
		sqlite3_close(db);
		return -1;
	}
}

/* 用户登录时，检查用户名和密码是否符合 */
struct online_info* check_table(char *userID, char *passwd, struct online_info* q) {
	sqlite3 *db;
	char *zErrMsg = 0;
	int rc;
	char sql[1024];

	rc = sqlite3_open(TABLE_NAME,&db);
	if(rc) {
		fprintf(stderr,"Can't open database in check_table: %s\n",sqlite3_errmsg(db));
		exit(0);
	}
	else
		fprintf(stdout,"Opened database successfully in check_table!\n");

	/* 查询用户名和密码是否匹配且是否是离线状态，防止登录了两次（异地登录） */
	sprintf(sql,"SELECT * FROM PLAYER WHERE ID = '%s' AND PASSWD = '%s' AND STATE = 0;",userID,passwd);

	int num = 0;
	rc = sqlite3_exec(db,sql,check_callback,&num,&zErrMsg);
	if(rc != SQLITE_OK) {
		fprintf(stderr,"SQL error in check_table: %s\n",zErrMsg);
		sqlite3_free(zErrMsg);
	} else
		fprintf(stdout, "status operation done successfully in check_table\n");
	
	//用户名和密码不匹配
	if(num != 1) {
		sqlite3_close(db);
		return NULL;
	}

	//用户名和密码匹配,查询所有在线用户的信息，并填充
	else {
		memset(sql,0,1024);
		sprintf(sql,"SELECT * FROM PLAYER WHERE STATE != 0");

		rc = sqlite3_exec(db,sql,login_callback,(void*)q,&zErrMsg);
		if(rc != SQLITE_OK) {
			fprintf(stderr,"SQL error in check_table when fill info: %s\n",zErrMsg);
			sqlite3_free(zErrMsg);
		}
		else
			fprintf(stdout, "Operation done successfully in check_table when fill info\n");

		//检查是否填充正确
		int j;
		for(j = 0; j < 200; j++)
			if(q->data[j] != '\0')
				printf("%c",q->data[j]);
			else
				printf("*");
		printf("\n");
		//printf("online player and info is : %s \n",q->data);

		//修改该用户状态
		memset(sql,0,1024);
		sprintf(sql,"UPDATE PLAYER SET STATE = 1 where ID = '%s';",userID);
		rc = sqlite3_exec(db,sql,NULL,0,&zErrMsg);
		if(rc != SQLITE_OK) {
			fprintf(stderr,"SQL error in check_table when update state: %s\n",zErrMsg);
			sqlite3_free(zErrMsg);
		} else
			fprintf(stdout,"Operation done successfully when update in check_table\n");

		sqlite3_close(db);
		return q;
	}

}

/* 当用户退出或进入游戏时，修改用户状态 */
int alter_table(char *userID, int status) {
	sqlite3 *db;
	char *zErrMsg = 0;
	int rc;
	char sql[1024]; 
	memset(sql,0,1024);

	/*Open database*/
	rc = sqlite3_open(TABLE_NAME,&db);
	if(rc) {
		fprintf(stderr,"Can't open database in alter_table: %s\n",sqlite3_errmsg(db));
		exit(0);
	}
	else
		fprintf(stdout,"Opened database successfully in alter_table!\n");

	sprintf(sql,"UPDATE PLAYER SET STATE = %d where ID = '%s'",status,userID);

	/* Execute SQL statement */
	rc = sqlite3_exec(db,sql,NULL,0,&zErrMsg);
	if(rc != SQLITE_OK) {
		fprintf(stderr,"SQL error in alter_table: %s\n",zErrMsg);
		sqlite3_free(zErrMsg);
		return -1;
	} else 
		fprintf(stdout, "Operation done successfully in alter_table\n");

	sqlite3_close(db);
	return 0;
}

#include <unistd.h>
#include <proxy.h>

int count_online(void)
{
	const char sql[] = "SELECT COUNT(*) FROM PLAYER WHERE STATE >= 1";
	sqlite3 *db;
	sqlite3_open(TABLE_NAME, &db);
	sqlite3_exec(db, sql, NULL, NULL, NULL);
	sqlite3_stmt *stmt;
	sqlite3_prepare_v2(db, sql, sizeof(sql) - 1, &stmt, NULL);
	sqlite3_step(stmt);
	int n = sqlite3_column_int(stmt, 0);
	sqlite3_step(stmt);
	return n;
}

typedef struct {
	int socket_fd;
	char username[10];
	uint8_t entry_state;
} ConnectionInfo;

//登陆时调用，发送所有在线用户
static int send_list_entry_callback(void *p, int argc, char **argv, char **col_name)
{
	ConnectionInfo *u = p;
	for (int i = 0; i < argc; i++) {
		if (strcmp(u->username, argv[i])) {
			PlayerEntry entry = { .state = u->entry_state };
			strncpy(entry.userID, argv[i], sizeof(entry.userID) - 1);
			write(u->socket_fd, &entry, sizeof(entry));
		}
	}
	return 0;
}

/**
 * @brief 查询数据库，逐个发送用户信息给新登陆的用户
 * @param connection 连接套接字
 * @return 发送的数目
 */
int send_list(int socket_fd, char username[])
{
	sqlite3 *db;
	sqlite3_open(TABLE_NAME, &db);

	ConnectionInfo info = { .socket_fd = socket_fd, .entry_state = 0 };
	strncpy(info.username, username, sizeof(info.username) - 1);

	const char sql_idle[] = "SELECT ID FROM PLAYER WHERE STATE = 1";
	const char sql_battle[] = "SELECT ID FROM PLAYER WHERE STATE = 2";

	sqlite3_exec(db, sql_idle, send_list_entry_callback, &info, NULL);
	info.entry_state = ENT_STATE_BUSY;
	sqlite3_exec(db, sql_battle, send_list_entry_callback, &info, NULL);
	return 0;
}
