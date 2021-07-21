#include <string>
#include "sqlite3.h"

#define CHAT_DB_DLL_EXPORT 1
#include "chatdb.h"
#include "chatdb_sys.h"

int chatDbActivities_( void* db, char *user, ChatDbActivitiesCallBack_ cb, void *customData ) 
{
	int ret_val = _error_ret_val;

	if( db == nullptr ) {
		return ret_val;
	}
	sqlite3 *_db = (sqlite3 *)db;
	
	sqlite3_stmt *statement;
	std::string r = 
		"SELECT chat.activity as activity, COUNT(chat.datetime) as count, MAX(chat.datetime) AS updated_at,"\
		" CASE WHEN users.datetime IS NULL OR users.datetime < MAX(chat.datetime) THEN 'y' ELSE 'n' END AS has_new"\
		" FROM chat"\
		" LEFT JOIN users ON users.user=? AND users.activity=chat.activity"\
		" GROUP BY chat.activity";

	int status = sqlite3_prepare( _db, r.c_str(), -1, &statement, NULL );
  if (status == SQLITE_OK) {
  	sqlite3_bind_text(statement, 1, user, strlen(user), SQLITE_STATIC);    

		while((status = sqlite3_step(statement)) == SQLITE_ROW) {
			char *activity = (char*)sqlite3_column_text( statement, 0 );
			unsigned int count = sqlite3_column_int( statement, 1 );
			unsigned int updated_at = sqlite3_column_int( statement, 2 );
			char *has_new = (char*)sqlite3_column_text( statement, 3 );
			cb( activity, count, updated_at, has_new, customData );
		}
		if( status == SQLITE_DONE ) {
			ret_val = _ok_ret_val;
		} 
		sqlite3_finalize(statement);
	}
	return ret_val;
}


int chatDbUpdateUserRead_( void* db, char *user, char *activity, unsigned int dt ) {
	int ret_val = _error_ret_val;
	int status;

	if( db == nullptr ) {
		return ret_val;
	}
	sqlite3* _db = (sqlite3 *)db;

	//	INSERT OR REPLACE INTO users (rowid, user, activity, datetime)
	//		VALUES ( (SELECT rowid FROM users where user='user' AND activity='activity' LIMIT 1), 'user', 'activity', 1234567890 );

	std::string r = "INSERT OR REPLACE INTO users (rowid, user, activity, datetime)"\
		" VALUES ( (SELECT rowid FROM users where user=? AND activity=? LIMIT 1), ?, ?, ? );";

	sqlite3_stmt *statement;
    
  status = sqlite3_prepare(_db, r.c_str(), -1, &statement, 0);
  if (status != SQLITE_OK) {
		return ret_val;
  }    
    
  sqlite3_bind_text(statement, 1, user, strlen(user), SQLITE_STATIC);    
  sqlite3_bind_text(statement, 2, activity, strlen(activity), SQLITE_STATIC);    
  sqlite3_bind_text(statement, 3, user, strlen(user), SQLITE_STATIC);    
  sqlite3_bind_text(statement, 4, activity, strlen(activity), SQLITE_STATIC);    
  sqlite3_bind_int(statement, 5, dt);        
  
	status = sqlite3_step(statement);
  if( status == SQLITE_DONE ) {
		ret_val = _ok_ret_val;
  }
        
  sqlite3_finalize(statement);    
	return ret_val;
}


/*
int __chatDbActivities_( void* db,  ChatDbActivitiesCallBack_ cb, void *customData ) 
{
	int ret_val = -1;
	if( db == nullptr ) {
		return ret_val;
	}
	sqlite3 *_db = (sqlite3 *)db;
	
	sqlite3_stmt *statement;
	std::string r = "SELECT activity, COUNT(rowid) as activity_count FROM `" + _chat_table + "`  GROUP BY activity;";

	int status = sqlite3_prepare( _db, r.c_str(), r.size(), &statement, NULL );
	if( status == SQLITE_OK ) {
		while((status = sqlite3_step(statement)) == SQLITE_ROW) {
			char *a = (char*)sqlite3_column_text( statement, 0 );
			unsigned int aCount = sqlite3_column_int( statement, 1 );
			cb( a, aCount, customData );
		}
		if( status == SQLITE_DONE ) {
			ret_val = 0;
		} 
		sqlite3_finalize(statement);
	}
	return ret_val;
}
*/