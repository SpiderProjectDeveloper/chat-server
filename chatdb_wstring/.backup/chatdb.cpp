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

void* chatDbOpen( std::wstring& dbFileName ) {
	return chatDbOpen_( dbFileName.c_str() ); 
}

void* chatDbOpen_( const wchar_t *dbFileName ) {
	sqlite3 *_db = nullptr;
	int status = sqlite3_open16( dbFileName, &_db );
	if( status != SQLITE_OK ) {
		return nullptr;
	}
	return (void *)_db;
}

void chatDbClose( void *db ) {
	if( db == nullptr ) {
		return;
	}
	sqlite3* _db = (sqlite3 *)db;
	sqlite3_close(_db);
}

int chatDbWrite( void *db, std::wstring& activity, std::wstring& user, std::wstring& message, 
	unsigned long int& rowid, unsigned long int& write_time ) 
{
	return chatDbWrite_( db, activity.c_str(), user.c_str(), message.c_str(), rowid, write_time );
}

int chatDbWrite_( void *db, const wchar_t *activity, const wchar_t *user, const wchar_t *message, 
	unsigned long int& rowid, unsigned long int& write_time ) 
{
	int ret_val = -1;
	if( db == nullptr ) {
		return ret_val;
	}
	sqlite3* _db = (sqlite3 *)db;
	if( !is_table_exists(_db) ) {
		if( create_table(_db) != 0 ) {
			return ret_val;
		}
	}

	sqlite3_stmt *statement;
	write_time = time(NULL);
	
	std::wstring r = L"INSERT INTO '" + _chat_table + L"' ('user', 'message', 'activity', 'datetime' )" + 
		L" VALUES ('" + user+ L"','" + message + L"','" + activity + L"'," + std::to_wstring(write_time) + L");";


	int status = sqlite3_prepare16( _db, r.c_str(), -1, &statement, NULL );
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


int chatDbWriteWithImage( void* db, std::wstring& activity, std::wstring& user, std::wstring &message, 
	std::string& icon, std::string& image, int width, int height, unsigned long int& rowid, unsigned long int& write_time ) 
{
	return chatDbWriteWithImage_( db, activity.c_str(), user.c_str(), message.c_str(), 
		icon.c_str(), image.c_str(), width, height, rowid, write_time );
}

int chatDbWriteWithImage_( void* db, const wchar_t *activity, const wchar_t *user, const wchar_t *message, 
	const char* icon, const char* image, int width, int height, unsigned long int& rowid, unsigned long int& write_time ) 
{
	int ret_val = -1;
	if( db == nullptr ) {
		return ret_val;
	}
	sqlite3* _db = (sqlite3 *)db;
	if( !is_table_exists(_db) ) {
		if( create_table(_db) != 0 ) {
			return ret_val;
		}
	}

	sqlite3_stmt *statement;
	write_time = time(NULL);

	std::wstring r = L""; 	// = L"BEGIN TRANSACTION;";
	//r += L"INSERT INTO " + _images_table + L" ('width', 'height', 'image')" + L" VALUES (" + 
	//	std::to_wstring(width) + L", " + std::to_wstring(height) + L", ? );";
	
	r += L"INSERT INTO `" + _chat_table + L"` ('user', 'message', 'activity', 'datetime', 'imageIcon', 'imageId' )" + 
			L" VALUES ('" + user+ L"','" + message + L"','" + activity + L"'," + std::to_wstring(write_time) + L", ?, LAST_INSERT_ROWID());";
std::wcout << L"r=" << r << std::endl;
	//r += L"COMMIT;";


	int status = sqlite3_prepare16( _db, r.c_str(), -1, &statement, NULL );
	if( status != SQLITE_OK ) {
		return ret_val;
	}

	sqlite3_bind_blob(statement, 1, (void*)icon, sizeof(char)*strlen(icon), SQLITE_STATIC);

	status = sqlite3_step( statement );
	if( status == SQLITE_DONE ) {
		ret_val = 0;
	} 
	sqlite3_finalize(statement);

	rowid = sqlite3_last_insert_rowid(_db);
	
	return ret_val;
}


int chatDbRead( void *db, std::wstring& activity, unsigned int limit, unsigned int offset, unsigned int rowidGreaterThan, std::wstring& buffer ) {
	int ret_val = -1;
	if( db == nullptr ) {
		return ret_val;
	}
	sqlite3* _db = (sqlite3 *)db;

	std::wstring r;
	create_read_request_string( r, activity, limit, offset, rowidGreaterThan );
	int status = make_request( _db, r.c_str(), [](sqlite3_stmt *stmt, void *buffer, void *custom_data ) { 
			wchar_t *pmsg = (wchar_t*)sqlite3_column_text16( stmt, 1 );
			std::wstring msg = L"";
			int l = wcslen(pmsg);

			for( int i = 0 ; i < l ; i++ ) {
				if( pmsg[i] == L'\r' && i < l-1 && pmsg[i+1] == L'\n' ) {
					msg += L"<br>";
				} else if( pmsg[i] == L'\n' || pmsg[i] == L'\r' ) {
					msg += L"<br>";
				} else if( pmsg[i] == L'\t' ) {
					msg += L"    ";
				} else if( pmsg[i] == L'<' ) {
					msg += L"&lt;";
				} else if( pmsg[i] == L'>' ) {
					msg += L"&gt;";
				} else if( pmsg[i] == L'"' ) {
					msg += L"&quot;";
				} else {
					msg += pmsg[i];
				}
			}
			if( ((std::wstring *)buffer)->size() > 0 ) {
				*((std::wstring *)buffer) += L',';	
			}
			std::wstring usr = std::wstring( (wchar_t*)sqlite3_column_text16( stmt, 0 ) );
			std::wstring dt = std::to_wstring( sqlite3_column_int( stmt, 2 ) );
			std::wstring rowid = std::to_wstring( sqlite3_column_int( stmt, 3 ) );
			std::string icon = ""; //std::string( (char *)sqlite3_column_blob( stmt, 4 ) );
			std::wstring imageId = std::to_wstring( sqlite3_column_int( stmt, 5 ) );
			*((std::wstring *)buffer) +=  L"{ \"usr\":\"" + usr + L"\", \"msg\":\"" + msg + 
				L"\", \"dt\": " + dt + L", \"rowid\":" + rowid + L", icon:\"" + L"\", \"imageId\":" + imageId + L"}";
		}, (void *)&buffer, nullptr
	);
	return status;
}

int chatDbReadCb( void* db, std::wstring& activity, 
	unsigned int limit, unsigned int offset, unsigned int rowidGreaterThan, ChatDbReadCallBack cb, void *customData ) 
{
	int ret_val = -1;
	if( db == nullptr ) {
		return ret_val;
	}
	sqlite3* _db = (sqlite3 *)db;
	
	std::wstring r;
	create_read_request_string( r, activity, limit, offset, rowidGreaterThan );
	int status = make_request( _db, r.c_str(), [](sqlite3_stmt *stmt, void *pvcb, void *customData ) { 
			std::wstring usr = std::wstring( (wchar_t*)sqlite3_column_text16( stmt, 0 ) );
			std::wstring msg = std::wstring( (wchar_t*)sqlite3_column_text16( stmt, 1 ) );
			unsigned int dt = sqlite3_column_int( stmt, 2 );
			unsigned int rowid = sqlite3_column_int( stmt, 3 );
			ChatDbReadCallBack cb = (ChatDbReadCallBack)pvcb;
			cb( usr, msg, dt, rowid, customData ); 				
		}, 
		(void *)cb, 
		customData
	);
	return status;		
}

int chatDbReadCb_( void* db, wchar_t *activity, 
	unsigned int limit, unsigned int offset, unsigned int rowidGreaterThan, ChatDbReadCallBack_ cb, void *customData ) 
{
	int ret_val = -1;
	if( db == nullptr ) {
		return ret_val;
	}
	sqlite3* _db = (sqlite3 *)db;
	
	std::wstring r;
	create_read_request_string( r, std::wstring(activity), limit, offset, rowidGreaterThan );
	int status = make_request( _db, r.c_str(), [](sqlite3_stmt *stmt, void *pvcb, void *customData ) { 
			wchar_t *usr = (wchar_t*)sqlite3_column_text16( stmt, 0 );
			wchar_t *msg = (wchar_t*)sqlite3_column_text16( stmt, 1 );
			unsigned int dt = sqlite3_column_int( stmt, 2 );
			unsigned int rowid = sqlite3_column_int( stmt, 3 );
			ChatDbReadCallBack_ cb = (ChatDbReadCallBack_)pvcb;
			cb( usr, msg, dt, rowid, customData ); 				
		}, 
		(void *)cb,
		customData
	);
	return status;		
}


int chatDbUpdate( void* db, std::wstring &message, unsigned int rowid ) {
	return chatDbUpdate_( db, message.c_str(), rowid );
}

int chatDbUpdate_( void* db, const wchar_t *message, unsigned int rowid ) {
		int ret_val = -1;
		if( db == nullptr ) {
			return ret_val;
		}
		sqlite3* _db = (sqlite3 *)db;

		sqlite3_stmt	*statement;
		std::wstring r = L"UPDATE '" + _chat_table + L"' SET message='" + message + L"' WHERE ROWID=" + std::to_wstring(rowid) + L";";
		int status = sqlite3_prepare16( _db, r.c_str(), -1, &statement, NULL );
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


int chatDbRemove( void* db, unsigned int rowid ) {
		int ret_val = -1;
		if( db == nullptr ) {
			return ret_val;
		}
		sqlite3* _db = (sqlite3 *)db;

		sqlite3_stmt	*statement;
		std::wstring r = L"DELETE FROM '" + _chat_table + L"' WHERE ROWID=" + std::to_wstring(rowid) + L";";
		int status = sqlite3_prepare16( _db, r.c_str(), -1, &statement, NULL );
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

	int status = sqlite3_prepare16( _db, request, -1, &statement, NULL );
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
		L" (width INT, height INT, type TEXT, image BLOB);";
	status = make_request( _db, r.c_str(), nullptr, nullptr, nullptr );

	r = L"CREATE TABLE IF NOT EXISTS " + _chat_table + 
		L" (user TEXT NOT NULL, message TEXT NOT NULL, activity TEXT NOT NULL, datetime INT NOT NULL, imageIcon BLOB, imageId INT," +
		L" CONSTRAINT fkImageId FOREIGN KEY(imageId) REFERENCES ROWID(" + _images_table + L") ON DELETE CASCADE);";
	status = make_request( _db, r.c_str(), nullptr, nullptr, nullptr );

	r = L"CREATE INDEX 'datetime_index' ON " + _chat_table + L" (`datetime` DESC)";
	status = make_request( _db, r.c_str(), nullptr, nullptr, nullptr );

	r = L"CREATE INDEX 'activity_index' ON " + _chat_table + L" (`activity` DESC)";
	status = make_request( _db, r.c_str(), nullptr, nullptr, nullptr );
	return status;
}


static void create_read_request_string( std::wstring& r, std::wstring& activity, 
	unsigned int limit, unsigned int offset, unsigned int rowidGreaterThan ) 
{
	if( rowidGreaterThan == -1 ) {
		r = L"SELECT user, message, datetime, ROWID, imageIcon, imageId FROM `" + _chat_table + L"`" + 
			L" WHERE activity='" + activity + L"' ORDER BY datetime DESC" + 
			L" LIMIT " + std::to_wstring(limit) + L" OFFSET " + std::to_wstring(offset) + L";";
	} else {
		r = L"SELECT user, message, datetime, ROWID, imageIcon, imageId FROM `" + _chat_table + L"`" +
			L" WHERE activity='" + activity + L"' AND ROWID>" + std::to_wstring(rowidGreaterThan) + L" ORDER BY datetime DESC" + 
			L" LIMIT " + std::to_wstring(limit) + L" OFFSET " + std::to_wstring(offset) + L";";
	}
	std::wcout << L"r=" << r << std::endl;
}