#include <stdio.h>
#include <windows.h>
#include <string>
#include <iostream>
#include <time.h>
#include <functional>
#include "sqlite3.h"

#define CHAT_DB_DLL_EXPORT 1
#include "chatdb.h"
#include "chatdb_sys.h"

static int make_request( 
	sqlite3 *, const char *request, void (*callback)( sqlite3_stmt *, void *, void *), 
	void *callback_data, void *custom_data 
);
static bool is_table_exists( sqlite3 * );
static int create_table( sqlite3 * );
static void  create_read_request_string( std::string& r, std::string& activity, 
	unsigned int limit, unsigned int offset, unsigned int rowidGreaterThan );

void* chatDbOpen_( const wchar_t *dbFileName ) {
	sqlite3 *_db = nullptr;
	int status = sqlite3_open16( dbFileName, &_db );
	if( status != SQLITE_OK ) {
		return nullptr;
	}
	return (void *)_db;
}

void chatDbClose_( void *db ) {
	if( db == nullptr ) {
		return;
	}
	sqlite3* _db = (sqlite3 *)db;
	sqlite3_close(_db);
}

int chatDbWrite_( void *db, const char *activity, const char *user, const char *message, 
	unsigned long int& rowid, unsigned long int& write_time ) 
{
	int ret_val = -1;
	if( db == nullptr ) {
		return ret_val;
	}
	sqlite3 *_db = (sqlite3 *)db;
	if( !is_table_exists(_db) ) {
		if( create_table(_db) != 0 ) {
			return ret_val;
		}
	}

	sqlite3_stmt *statement;
	write_time = time(NULL);
	std::string r = "INSERT INTO '" + _chat_table + "' ('user', 'message', 'activity', 'datetime')" + 
		" VALUES ('" + user + "','" + message + "','" + activity + "'," + std::to_string(write_time) + ");";
	const char *sql = r.c_str();
	int status = sqlite3_prepare_v2( _db, sql, strlen(sql)+1, &statement, NULL );
	if( status != SQLITE_OK ) {
		return ret_val;
	}
	status = sqlite3_step( statement );
	if( status == SQLITE_DONE ) {
		ret_val = 0;
	} 
	sqlite3_finalize(statement);

	rowid = sqlite3_last_insert_rowid(_db);

	chatDbUpdateUserRead_( _db, user, activity, write_time );

	return ret_val;
}

static int beginTransaction( sqlite3* _db ) {
	return ( sqlite3_exec(_db, "BEGIN TRANSACTION;", NULL, NULL, NULL) == SQLITE_OK ) ? 0 : -1;
}

static int commitTransaction( sqlite3* _db ) {
	return ( sqlite3_exec(_db, "END TRANSACTION;", NULL, NULL, NULL) == SQLITE_OK ) ? 0 : -1;
}

static int rollbackTransaction( sqlite3* _db ) {
	return ( sqlite3_exec(_db, "ROLLBACK;", NULL, NULL, NULL) == SQLITE_OK ) ? 0 : -1;
}

int chatDbWriteWithImage_( void* db, const char *activity, const char *user, const char *message, 
	const char* icon, const char* image, int width, int height, unsigned long int& rowid, unsigned long int& write_time ) 
{
	int error_ret_val = -1, ok_ret_val = 0;
	int status;

	if( db == nullptr ) {
		return error_ret_val;
	}
	sqlite3* _db = (sqlite3 *)db;
	if( !is_table_exists(_db) ) {
		if( create_table(_db) != 0 ) {
			return error_ret_val;
		}
	}

	if( beginTransaction(_db) != 0 ) { return error_ret_val; }

	bool imgInserted = false;
	std::string rimg = "INSERT INTO `" + _images_table + "` ('width', 'height', 'image')" + " VALUES (" + 
		std::to_string(width) + ", " + std::to_string(height) + ", \"" + image + "\" );";	
	if( sqlite3_exec( _db, rimg.c_str(), NULL, NULL, NULL ) == SQLITE_OK ) {
			imgInserted	= true;
	} 
	if( !imgInserted ) {
		rollbackTransaction(_db);
		return error_ret_val;
	}

	bool msgInserted = false;
	int imageRowId = sqlite3_last_insert_rowid(_db);
	write_time = time(NULL);
	std::string rmsg = "INSERT INTO `" + _chat_table + "` ('user', 'message', 'activity', 'datetime', 'imageIcon', 'imageId' )" + 
			" VALUES (\"" + user + "\", \"" + message + "\",\"" + activity + "\"," + 
				std::to_string(write_time) + ",\"" + icon + "\"," + std::to_string(imageRowId) + ");";
	if( sqlite3_exec( _db, rmsg.c_str(), NULL, NULL, NULL ) == SQLITE_OK ) {
			msgInserted	= true;
	} 
	if( !msgInserted ) {
		rollbackTransaction(_db);
		return error_ret_val;
	}
	rowid = sqlite3_last_insert_rowid(_db);
	
	if( commitTransaction(_db) != 0 ) { return error_ret_val; }

	chatDbUpdateUserRead_( _db, user, activity, write_time );

	return ok_ret_val;
}


int chatDbRead_( 
	void* db, char *activity, unsigned int limit, unsigned int offset, unsigned int rowidGreaterThan, 
	ChatDbReadCallBack_ cb, void *customData ) 
{
	int ret_val = -1;
	if( db == nullptr ) {
		return ret_val;
	}
	sqlite3 *_db = (sqlite3 *)db;
	
	std::string r;
	create_read_request_string( r, std::string(activity), limit, offset, rowidGreaterThan );
	int status = make_request( _db, r.c_str(), [](sqlite3_stmt *stmt, void *pvcb, void *customData ) { 
			char *usr = (char*)sqlite3_column_text( stmt, 0 );
			char *msg = (char*)sqlite3_column_text( stmt, 1 );
			unsigned int dt = sqlite3_column_int( stmt, 2 );
			unsigned int rowid = sqlite3_column_int( stmt, 3 );
			char *icon = (char*)sqlite3_column_text( stmt, 4 );
			unsigned int imageId = sqlite3_column_int( stmt, 5 );

			ChatDbReadCallBack_ cb = (ChatDbReadCallBack_)pvcb;
			cb( usr, msg, dt, rowid, icon, imageId, customData ); 				
		}, 
		(void *)cb,
		customData
	);
	return status;		
}

int chatDbReadId_( void* db, unsigned int rowid, ChatDbReadCallBack_ cb, void *customData ) 
{
	int ret_val = -1;
	if( db == nullptr ) {
		return ret_val;
	}
	sqlite3 *_db = (sqlite3 *)db;
	
	std::string r = "SELECT user, message, datetime, ROWID, imageIcon, imageId FROM `" + _chat_table + "`" + 
			" WHERE ROWID=" + std::to_string(rowid) + " LIMIT 1;";
	int status = make_request( _db, r.c_str(), [](sqlite3_stmt *stmt, void *pvcb, void *customData ) { 
			char *usr = (char*)sqlite3_column_text( stmt, 0 );
			char *msg = (char*)sqlite3_column_text( stmt, 1 );
			unsigned int dt = sqlite3_column_int( stmt, 2 );
			unsigned int rowid = sqlite3_column_int( stmt, 3 );
			char *icon = (char*)sqlite3_column_text( stmt, 4 );
			unsigned int imageId = sqlite3_column_int( stmt, 5 );

			ChatDbReadCallBack_ cb = (ChatDbReadCallBack_)pvcb;
			cb( usr, msg, dt, rowid, icon, imageId, customData ); 				
		}, 
		(void *)cb,
		customData
	);
	return status;		
}


int chatDbReadImage_( void* db, unsigned int rowid, ChatDbReadImageCallBack_ cb, void *customData ) 
{
	int ret_val = -1;
	if( db == nullptr ) {
		return ret_val;
	}
	sqlite3 *_db = (sqlite3 *)db;
	
	std::string r = "SELECT image, width, height FROM `" +_images_table + "` WHERE ROWID=" + std::to_string(rowid) + " LIMIT 1;";
	int status = make_request( _db, r.c_str(), [](sqlite3_stmt *stmt, void *pvcb, void *customData ) { 
			char *image = (char*)sqlite3_column_text( stmt, 0 );
			unsigned int width = sqlite3_column_int( stmt, 1 );
			unsigned int height = sqlite3_column_int( stmt, 2 );
			ChatDbReadImageCallBack_ cb = (ChatDbReadImageCallBack_)pvcb;
			cb( image, width, height, customData ); 				
		}, 
		(void *)cb,
		customData
	);
	return status;		
}

int chatDbUpdate_( void* db, const char *message, unsigned int rowid ) {
		int ret_val = -1;
		if( db == nullptr ) {
			return ret_val;
		}
		sqlite3* _db = (sqlite3 *)db;

		sqlite3_stmt *statement;
		std::string r = "UPDATE '" + _chat_table + "' SET message='" + message + "' WHERE ROWID=" + std::to_string(rowid) + ";";
		const char *sql = r.c_str();
		int status = sqlite3_prepare_v2( _db, sql, strlen(sql)+1, &statement, NULL );
		if( status != SQLITE_OK ) {
			return ret_val;
		}
		status = sqlite3_step( statement );
		if( status == SQLITE_DONE ) {
			ret_val = 0;
		} 
		sqlite3_finalize(statement);
		return ret_val;
}


int chatDbUpdateWithImage_( void* db, const char *message, const char *icon, 
	unsigned int rowid, const char *image, unsigned int width, unsigned int height, unsigned int imageId ) 
{
	int error_ret_val = -1, ok_ret_val = 0;
	int status;
	std::string r;
	if( db == nullptr ) {
		return error_ret_val;
	}
	sqlite3* _db = (sqlite3 *)db;

	if( beginTransaction(_db) != 0 ) { return error_ret_val; }

	if( image == nullptr || strlen(image) == 0 ) {
		r = "DELETE FROM `" + _images_table + "` WHERE ROWID=" + std::to_string(imageId) + ";";
	} else {
		if( imageId <= 0 ) {
			r = "INSERT INTO `" + _images_table + "` ('width', 'height', 'image')" + " VALUES (" + 
				std::to_string(width) + ", " + std::to_string(height) + ", \"" + image + "\" );";	
		} else {
			r = "UPDATE `" + _images_table + "` SET image=\"" + image + "\", width=" + std::to_string(width) + 
				", height=" + std::to_string(height) + " WHERE ROWID=" + std::to_string(imageId) + ";";
		}
	}
	status = sqlite3_exec( _db, r.c_str(), NULL, NULL, NULL );
	if( status != SQLITE_OK ) {
		rollbackTransaction(_db);
		return error_ret_val;
	}

	if( imageId <= 0 ) {
		imageId = sqlite3_last_insert_rowid(_db);
	}
	if( imageId <= 0 ) {
		rollbackTransaction(_db);
		return error_ret_val;
	}

	if( image == nullptr || strlen(image) == 0 ) {
		r = "UPDATE `" + _chat_table + "` SET message=\"" + message + "\", imageIcon=NULL, imageId=NULL" + 
			" WHERE ROWID=" + std::to_string(rowid) + ";";
	} else {
		r = "UPDATE `" + _chat_table + "` SET message=\"" + message + "\", imageIcon=\"" + icon + "\"" +
		", imageId=" + std::to_string(imageId) + " WHERE ROWID=" + std::to_string(rowid) + ";";
	}
	status = sqlite3_exec( _db, r.c_str(), NULL, NULL, NULL );
	if( status != SQLITE_OK ) {
		rollbackTransaction(_db);
		return error_ret_val;
	}

	if( commitTransaction(_db) != 0 ) { return error_ret_val; }
	return ok_ret_val;
}

int chatDbRemove_( void* db, unsigned int rowid ) {
	int error_ret_val = -1, ok_ret_val=0;
	if( db == nullptr ) {
		return error_ret_val;
	}
	sqlite3* _db = (sqlite3 *)db;

	std::string r = "DELETE FROM '" + _chat_table + "' WHERE ROWID=" + std::to_string(rowid) + ";";
	int status = sqlite3_exec( _db, r.c_str(), NULL, NULL, NULL );
	if( status != SQLITE_OK ) {
		return error_ret_val;
	}
	return ok_ret_val;
}

int chatDbRemoveWithImage_( void* db, unsigned int rowid, unsigned int imageId ) {
	int error_ret_val = -1, ok_ret_val=0;
	int status;
	std::string r;

	if( db == nullptr ) {
		return error_ret_val;
	}
	sqlite3* _db = (sqlite3 *)db;

	if( imageId < 0 ) {		// No image
		std::string r = "DELETE FROM '" + _chat_table + "' WHERE ROWID=" + std::to_string(rowid) + ";";
		const char *sql = r.c_str();
		int status = sqlite3_exec( _db, sql, NULL, NULL, NULL );
		if( status != SQLITE_OK ) {
			return error_ret_val;
		}
		return ok_ret_val;
	}

	if( beginTransaction(_db) != 0 ) { return error_ret_val; }

	r = "DELETE FROM '" + _chat_table + "' WHERE ROWID=" + std::to_string(rowid) + ";";
	status = sqlite3_exec( _db, r.c_str(), NULL, NULL, NULL );
	if( status != SQLITE_OK ) {
		rollbackTransaction(_db);
		return error_ret_val;
	}

	r = "DELETE FROM '" + _images_table + "' WHERE ROWID=" + std::to_string(imageId) + ";";
	status = sqlite3_exec( _db, r.c_str(), NULL, NULL, NULL );
	if( status != SQLITE_OK ) {
		rollbackTransaction(_db);
		return error_ret_val;
	}
	
	if( commitTransaction(_db) != 0 ) { return error_ret_val; }
	return ok_ret_val;
}


static int make_request( sqlite3 *_db, const char *request, 
	void (*callback)( sqlite3_stmt *statement, void *callback_data, void *custom_data), void *callback_data, void *custom_data ) 
{
	int ret_val = -1;
	sqlite3_stmt *statement;

	int status = sqlite3_prepare( _db, request, strlen(request)+1, &statement, NULL );
	if( status == SQLITE_OK ) {
		while((status = sqlite3_step(statement)) == SQLITE_ROW) {
			if( callback != nullptr ) {
				callback( statement, callback_data, custom_data );
			}
		}
		if( status == SQLITE_DONE ) {
			ret_val = 0;
		} 
		sqlite3_finalize(statement);
	}
	return ret_val;
}


static bool is_table_exists( sqlite3 *_db ) {
	bool exists = false;
	std::string r = "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='" + _chat_table + "';";
	int status = make_request( _db, r.c_str(), [](sqlite3_stmt *stmt, void *cbdata, void *custom_data) { 
			*((bool *)cbdata) = (sqlite3_column_int( stmt, 0 ) == 1); 
			return;
		}, (void *)&exists, nullptr
	);
	return (status == 0 && exists);
}		


static int create_table( sqlite3* _db ) {
	int ok_ret_val = 0, error_ret_val = -1;
	int status;
	std::string r;

	if( beginTransaction(_db) != 0 ) { return error_ret_val; }

	// Images table
	r = "CREATE TABLE IF NOT EXISTS " + _images_table + " (image TEXT NOT NULL, width INT, height INT, type TEXT );";
	status = make_request( _db, r.c_str(), nullptr, nullptr, nullptr );
	if( status != 0 ) {
		rollbackTransaction(_db);
		return error_ret_val;
	}

	// Chat table
	r = "CREATE TABLE IF NOT EXISTS " + _chat_table + 
		" (user TEXT NOT NULL, message TEXT NOT NULL, activity TEXT NOT NULL, datetime INT NOT NULL, imageIcon TEXT, imageId INT)";
	status = make_request( _db, r.c_str(), nullptr, nullptr, nullptr );
	if( status != 0 ) {
		rollbackTransaction(_db);
		return error_ret_val;
	}
	r = "CREATE INDEX IF NOT EXISTS 'datetime_index' ON " + _chat_table + " (`datetime` DESC)";
	status = make_request( _db, r.c_str(), nullptr, nullptr, nullptr );
	if( status != 0 ) {
		rollbackTransaction(_db);
		return error_ret_val;
	}
	r = "CREATE INDEX IF NOT EXISTS 'activity_index' ON " + _chat_table + " (`activity` DESC)";
	status = make_request( _db, r.c_str(), nullptr, nullptr, nullptr );
	if( status != 0 ) {
		rollbackTransaction(_db);
		return error_ret_val;
	}

	// Users table
	r = "CREATE TABLE IF NOT EXISTS " + _users_table + " (user TEXT NOT NULL, activity TEXT NOT NULL, datetime INT NOT NULL, UNIQUE(user, activity) ON CONFLICT REPLACE);";
	status = make_request( _db, r.c_str(), nullptr, nullptr, nullptr );
	if( status != 0 ) {
		rollbackTransaction(_db);
		return error_ret_val;
	}
	r = "CREATE INDEX IF NOT EXISTS 'datetime_index' ON " + _users_table + " (`datetime` DESC)";
	status = make_request( _db, r.c_str(), nullptr, nullptr, nullptr );
	if( status != 0 ) {
		rollbackTransaction(_db);
		return error_ret_val;
	}
	r = "CREATE INDEX IF NOT EXISTS 'activity_index' ON " + _users_table + " (`activity` DESC)";
	status = make_request( _db, r.c_str(), nullptr, nullptr, nullptr );
	if( status != 0 ) {
		rollbackTransaction(_db);
		return error_ret_val;
	}

	if( commitTransaction(_db) != 0 ) { return error_ret_val; }

	return ok_ret_val;
}


static void create_read_request_string( std::string& r, std::string& activity, 
	unsigned int limit, unsigned int offset, unsigned int rowidGreaterThan ) 
{
	if( rowidGreaterThan == -1 ) {
		r = "SELECT user, message, datetime, ROWID, imageIcon, imageId FROM `" + _chat_table + "`" + 
			" WHERE activity=\"" + activity + "\" ORDER BY datetime DESC" + 
			" LIMIT " + std::to_string(limit) + " OFFSET " + std::to_string(offset) + ";";
	} else {
		r = "SELECT user, message, datetime, ROWID, imageIcon, imageId FROM `" + _chat_table + "`" +
			" WHERE activity=\"" + activity + "\" AND ROWID>" + std::to_string(rowidGreaterThan) + " ORDER BY datetime DESC" + 
			" LIMIT " + std::to_string(limit) + " OFFSET " + std::to_string(offset) + ";";
	}
}
