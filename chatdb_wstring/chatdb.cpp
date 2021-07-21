#include <stdio.h>
#include <windows.h>
#include <string>
#include <iostream>
#include <time.h>
#include <functional>
#include "sqlite3.h"

#define CHAT_DB_DLL_EXPORT 1
#include "chatdb.h"

static const std::wstring _chat_table = L"chat";				
static const std::wstring _images_table = L"images";				

static int make_request( 
	sqlite3 *, const wchar_t *request, void (*callback)( sqlite3_stmt *, void *, void *), 
	void *callback_data, void *custom_data 
);
static bool is_table_exists( sqlite3 * );
static int create_table( sqlite3 * );
static void  create_read_request_string( std::wstring& r, std::wstring& activity, 
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

int chatDbWrite_( void *db, const wchar_t *activity, const wchar_t *user, const wchar_t *message, 
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
	std::wstring r = L"INSERT INTO '" + _chat_table + L"' ('user', 'message', 'activity', 'datetime')" + 
		L" VALUES ('" + user + L"','" + message + L"','" + activity + L"'," + std::to_wstring(write_time) + L");";
	const wchar_t *sql = r.c_str();
	int status = sqlite3_prepare16( _db, sql, wcslen(sql)+1, &statement, NULL );
	if( status != SQLITE_OK ) {
		return ret_val;
	}
	status = sqlite3_step( statement );
	if( status == SQLITE_DONE ) {
		ret_val = 0;
	} 
	sqlite3_finalize(statement);

	rowid = sqlite3_last_insert_rowid(_db);
	
	return ret_val;
}

static int beginTransaction( sqlite3* _db ) {
	return ( sqlite3_exec16(_db, L"BEGIN TRANSACTION;", NULL, NULL, NULL) == SQLITE_OK ) ? 0 : -1;
}

static int commitTransaction( sqlite3* _db ) {
	return ( sqlite3_exec16(_db, L"END TRANSACTION;", NULL, NULL, NULL) == SQLITE_OK ) ? 0 : -1;
}

static int rollbackTransaction( sqlite3* _db ) {
	return ( sqlite3_exec16(_db, L"ROLLBACK;", NULL, NULL, NULL) == SQLITE_OK ) ? 0 : -1;
}

int chatDbWriteImage_( void* db, const wchar_t *activity, const wchar_t *user, const wchar_t *message, 
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
	std::wstring rimg = L"INSERT INTO `" + _images_table + L"` ('width', 'height', 'image')" + L" VALUES (" + 
		std::to_wstring(width) + L", " + std::to_wstring(height) + L", \"" + image + L"\" );";	
	if( sqlite3_exec16( _db, rimg.c_str(), NULL, NULL, NULL ) == SQLITE_OK ) {
			imgInserted	= true;
	} 
	if( !imgInserted ) {
		rollbackTransaction(_db);
		return error_ret_val;
	}

	bool msgInserted = false;
	int imageRowId = sqlite3_last_insert_rowid(_db);
	write_time = time(NULL);
	std::wstring rmsg = L"INSERT INTO `" + _chat_table + L"` ('user', 'message', 'activity', 'datetime', 'imageIcon', 'imageId' )" + 
			L" VALUES (\"" + user + L"\", \"" + message + L"\",\"" + activity + L"\"," + 
				std::to_wstring(write_time) + L",\"" + icon + L"\"," + std::to_wstring(imageRowId) + ");";
	if( sqlite3_exec16( _db, rmsg.c_str(), NULL, NULL, NULL ) == SQLITE_OK ) {
			msgInserted	= true;
	} 
	if( !msgInserted ) {
		rollbackTransaction(_db);
		return error_ret_val;
	}
	rowid = sqlite3_last_insert_rowid(_db);
	
	if( commitTransaction(_db) != 0 ) { return error_ret_val; }

	return ok_ret_val;
}


int chatDbReadCb_( 
	void* db, wchar_t *activity, unsigned int limit, unsigned int offset, unsigned int rowidGreaterThan, 
	ChatDbReadCallBack_ cb, void *customData ) 
{
	int ret_val = -1;
	if( db == nullptr ) {
		return ret_val;
	}
	sqlite3 *_db = (sqlite3 *)db;
	
	std::wstring r;
	create_read_request_string( r, std::wstring(activity), limit, offset, rowidGreaterThan );
	int status = make_request( _db, r.c_str(), [](sqlite3_stmt *stmt, void *pvcb, void *customData ) { 
			wchar_t *usr = (wchar_t*)sqlite3_column_text( stmt, 0 );
			wchar_t *msg = (wchar_t*)sqlite3_column_text( stmt, 1 );
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

int chatDbReadImageCb_( void* db, unsigned int rowid, ChatDbReadImageCallBack_ cb, void *customData ) 
{
	int ret_val = -1;
	if( db == nullptr ) {
		return ret_val;
	}
	sqlite3 *_db = (sqlite3 *)db;
	
	std::wstring r = L"SELECT image, width, height FROM `" +_images_table + L"` WHERE ROWID=" + std::to_wstring(rowid) + L" LIMIT 1;";
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

int chatDbUpdate_( void* db, const wchar_t *message, unsigned int rowid ) {
		int ret_val = -1;
		if( db == nullptr ) {
			return ret_val;
		}
		sqlite3* _db = (sqlite3 *)db;

		sqlite3_stmt *statement;
		std::wstring r = L"UPDATE '" + _chat_table + L"' SET message='" + message + L"' WHERE ROWID=" + std::to_wstring(rowid) + ";";
		const wchar_t *sql = r.c_str();
		int status = sqlite3_prepare16( _db, sql, wcslen(sql)+1, &statement, NULL );
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


int chatDbRemove_( void* db, unsigned int rowid ) {
		int ret_val = -1;
		if( db == nullptr ) {
			return ret_val;
		}
		sqlite3* _db = (sqlite3 *)db;

		sqlite3_stmt	*statement;
		std::wstring r = L"DELETE FROM '" + _chat_table + L"' WHERE ROWID=" + std::to_wstring(rowid) + ";";
		const wchar_t *sql = r.c_str();
		int status = sqlite3_prepare16( _db, sql, wcslen(sql)+1, &statement, NULL );
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


static int make_request( sqlite3 *_db, const wchar_t *request, 
	void (*callback)( sqlite3_stmt *statement, void *callback_data, void *custom_data), void *callback_data, void *custom_data ) 
{
	int ret_val = -1;
	sqlite3_stmt *statement;

	int status = sqlite3_prepare16( _db, request, wscanf(request)+1, &statement, NULL );
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
	std::wstring r = L"SELECT count(*) FROM sqlite_master WHERE type='table' AND name='" + _chat_table + L"';";
	int status = make_request( _db, r.c_str(), [](sqlite3_stmt *stmt, void *cbdata, void *custom_data) { 
			*((bool *)cbdata) = (sqlite3_column_int( stmt, 0 ) == 1); 
			return;
		}, (void *)&exists, nullptr
	);
	return (status == 0 && exists);
}		


static int create_table( sqlite3* _db ) {
	int status;
	std::wstring r;

	r = L"CREATE TABLE IF NOT EXISTS " + _images_table + 
		L" (image TEXT NOT NULL, width INT, height INT, type TEXT );";
	status = make_request( _db, r.c_str(), nullptr, nullptr, nullptr );

	r = L"CREATE TABLE IF NOT EXISTS " + _chat_table + 
		L" (user TEXT NOT NULL, message TEXT NOT NULL, activity TEXT NOT NULL, datetime INT NOT NULL, imageIcon TEXT, imageId INT," +
		L" CONSTRAINT fkImageId FOREIGN KEY(imageId) REFERENCES ROWID(" + _images_table + ") ON DELETE CASCADE);";
	status = make_request( _db, r.c_str(), nullptr, nullptr, nullptr );

	r = L"CREATE INDEX 'datetime_index' ON " + _chat_table + " (`datetime` DESC)";
	status = make_request( _db, r.c_str(), nullptr, nullptr, nullptr );

	r = L"CREATE INDEX 'activity_index' ON " + _chat_table + " (`activity` DESC)";
	status = make_request( _db, r.c_str(), nullptr, nullptr, nullptr );
	return status;
}


static void create_read_request_string( std::wstring& r, std::wstring& activity, 
	unsigned int limit, unsigned int offset, unsigned int rowidGreaterThan ) 
{
	if( rowidGreaterThan == -1 ) {
		r = L"SELECT user, message, datetime, ROWID, imageIcon, imageId FROM `" + _chat_table + L"`" + 
			L" WHERE activity=\"" + activity + L"\" ORDER BY datetime DESC" + 
			L" LIMIT " + std::to_wstring(limit) + L" OFFSET " + std::to_wstring(offset) + ";";
	} else {
		r = L"SELECT user, message, datetime, ROWID, imageIcon, imageId FROM `" + _chat_table + L"`" +
			L" WHERE activity=\"" + activity + L"\" AND ROWID>" + std::to_wstring(rowidGreaterThan) + L" ORDER BY datetime DESC" + 
			L" LIMIT " + std::to_wstring(limit) + L" OFFSET " + std::to_wstring(offset) + L";";
	}
}
