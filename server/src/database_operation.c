#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

#define TABLE_NAME "player.db"

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
static int login_callback(void *data,int argc,char **argv,char **azColName) {
	int i;
	for(i = 0; i < argc; i++) {

	}
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
		fprintf(stderr,"Can't open database: %s\n",sqlite3_errmsg(db));
		exit(0);
	}
	else
		fprintf(stdout,"Opened database successfully!\n");

	/* Create SQL statement */
	sql = "CREATE TABLE PLAYER(" \
		   "ID CHAR(20) NOT NULL," \
		   "PASSWD CHAR(20) NOT NULL," \
		   "STATE  INT NOT NULL);";

	/* Execute SQL statement */
	rc = sqlite3_exec(db,sql,callback,0,&zErrMsg);
	if(rc != SQLITE_OK) {
		fprintf(stderr,"SQL error: %s\n",zErrMsg);
		sqlite3_free(zErrMsg);
	}
	else
		fprintf(stdout,"Table created successfully\n");

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
		fprintf(stderr,"Can't open database: %s\n",sqlite3_errmsg(db));
		exit(0);
	} else
		fprintf(stdout,"Opened database successfully!\n");

	/* check if has same UserID */
	sprintf(sql,"SELECT FROM PLAYER WHERE userID GLOB '%s';",userID);
	
	int num = 0;
	rc = sqlite3_exec(db,sql,register_callback,&num,&zErrMsg);
	if(rc != SQLITE_OK) {
		fprintf(stderr,"SQL error: %s\n",zErrMsg);
		sqlite3_free(zErrMsg);
	} else 
		fprintf(stdout, "Check operation done successfully\n");

	//没有相同的ID
	if(num == 1) {
		memset(sql,0,1024);
		sprintf(sql,"INSERT INTO PLAYER (ID,PASSWD,STATUS) VALUES('%s','%s',0)",userID,passwd);

		rc = sqlite3_exec(db,sql,callback,0,&zErrMsg);
		if(rc != SQLITE_OK) {
			fprintf(stderr,"SQL error: %s\n",zErrMsg);
			sqlite3_free(zErrMsg);
			sqlite3_close(db);
			return -1;
		} else {
			fprintf(stdout,"Insert done successfully\n");
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
char* check_table(char *userID, char *passwd, char *data) {
	sqlite3 *db;
	char *zErrMsg = 0;
	int rc;
	char sql[1024];

	rc = sqlite3_open(TABLE_NAME,&db);
	if(rc) {
		fprintf(stderr,"Can't open database: %s\n",sqlite3_errmsg(db));
		exit(0);
	}
	else
		fprintf(stdout,"check_table function Opened database successfully!\n");

	/* 查询用户名和密码是否匹配 */
	sprintf(sql,"SELECT * FROM PLAYER WHERE ID = '%s' AND PASSWD = '%s';",userID,passwd);

	int num;
	rc = sqlite3_exec(db,sql,check_callback,&num,&zErrMsg);
	if(rc != SQLITE_OK) {
		fprintf(stderr,"SQL error: %s\n",zErrMsg);
		sqlite3_free(zErrMsg);
	} else
		fprintf(stdout, "status operation done successfully\n");
	
	//用户名和密码不匹配
	if(num != 1) {
		sqlite3_close(db);
		return NULL;
	}

	//用户名和密码匹配,查询所有在线用户的信息，并填充
	else {
		memset(sql,0,1024);
		sprintf(sql,"SELECT * FROM PLAYER WHERE STATUS != 0");

		rc = sqlite3_exec(db,sql,login_callback,data,&zErrMsg);
		if(rc != SQLITE_OK) {
			fprintf(stderr,"SQL error: %s\n",zErrMsg);
			sqlite3_free(zErrMsg);
		}
		else
			fprintf(stdout, "status operation done successfully\n");

		sqlite3_close(db);
		return data;
	}

}

/* 当用户退出或进入游戏时，修改用户状态 */
int alter_table(char *userID, char *passwd, int status) {
	sqlite3 *db;
	char *zErrMsg = 0;
	int rc;
	char *sql; 

	/*Open database*/
	rc = sqlite3_open(TABLE_NAME,&db);
	if(rc) {
		fprintf(stderr,"Can't open database: %s\n",sqlite3_errmsg(db));
		exit(0);
	
	}
	else
		fprintf(stdout,"Opened database successfully!\n");

	sql = strcat("UPDATE PLAYER set status = 0 where ID = ",userID);
	sql = strcat(sql,"SELECT * from PLAYER");

	/* Execute SQL statement */
	rc = sqlite3_exec(db,sql,callback,0,&zErrMsg);
	if(rc != SQLITE_OK) {
		fprintf(stderr,"SQL error: %s\n",zErrMsg);
		sqlite3_free(zErrMsg);
	} else 
		fprintf(stdout, "Status operation done successfully\n");

	sqlite3_close(db);
	return 0;
}

