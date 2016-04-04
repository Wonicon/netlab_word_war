#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

#define TABLE_NAME "player.db"

//注册时调用，检查是否有ID相同的用户
static int register_callback(void *num,int argc, char **argv, char **azColName) {
	//num = (int *)num;
	*((int *)num) = (argc == 0) ? 1 : 0;
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
int insert_table(char *UserID,char *passwd) {
	sqlite3 *db;
	char *zErrMsg = 0;
	int rc;
	char *sql;

	/*Open database*/
	rc = sqlite3_open(TABLE_NAME,&db);
	if(rc)	{
		fprintf(stderr,"Can't open database: %s\n",sqlite3_errmsg(db));
		exit(0);
	} else
		fprintf(stdout,"Opened database successfully!\n");

	/* check if has same UserID */
	sql = strcat("SELECT FROM PLAYER WHERE UserID GLOB \'",UserID);
	sql = strcat(sql,"\'");
	
	int num;
	rc = sqlite3_exec(db,sql,register_callback,&num,&zErrMsg);
	if(rc != SQLITE_OK) {
		fprintf(stderr,"SQL error: %s\n",zErrMsg);
		sqlite3_free(zErrMsg);
	} else 
		fprintf(stdout, "Check operation done successfully\n");

	//没有相同的ID
	if(num == 1) {
		sql = strcat("INSERT INTO PLAYER (ID,PASSWD,STATUS) VALUES (\'",UserID);
		sql = strcat(sql,"\',\'");
		sql = strcat(sql,passwd);
		sql = strcat(sql,"\',");
		sql = strcat(sql,"0);");

		rc = sqlite3_exec(db,sql,callback,0,&zErrMsg);
		if(rc != SQLITE_OK) {
			fprintf(stderr,"SQL error: %s\n",zErrMsg);
			sqlite3_free(zErrMsg);
			return -1;
		} else {
			fprintf(stdout,"Insert done successfully\n");
			return 0;
		}

	}
	else         //有相同的ID
		return -1;
}

/* 当用户退出或进入游戏时，修改用户状态 */
int alter_table(char *UserID,int status) {
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

	sql = strcat("UPDATE PLAYER set status = 0 where ID = ",UserID);
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
