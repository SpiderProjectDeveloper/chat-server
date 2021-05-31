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

static int make_request( sqlite3 *, const wchar_t *request, void (*callback)( sqlite3_stmt *, void *), void *callback_data );
static bool is_table_exists( sqlite3 * );
static int create_table( sqlite3 * );
static void  create_read_request_string( std::wstring& r, std::wstring& activity, unsigned int limit, unsigned int offset );

void* chatDbOpen( std::wstring& dbFileName ) {
	sqlite3 *_db = nullptr;
	const wchar_t *dbfn = dbFileName.c_str();
	int status = sqlite3_open16( dbfn, &_db );
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

	sqlite3_stmt	*statement;
	write_time = time(NULL);
	std::wstring r = L"INSERT INTO '" + _chat_table + L"' ('user', 'message', 'activity', 'datetime')" + 
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

int chatDbRead( void *db, std::wstring& activity, unsigned int limit, unsigned int offset, std::wstring& buffer ) {
	int ret_val = -1;
	if( db == nullptr ) {
		return ret_val;
	}
	sqlite3* _db = (sqlite3 *)db;

	std::wstring r;
	create_read_request_string( r, activity, limit, offset );
	int status = make_request( _db, r.c_str(), [](sqlite3_stmt *stmt, void *buffer ) { 
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
			*((std::wstring *)buffer) +=  L"{ \"usr\":\"" + usr + L"\", \"msg\":\"" + msg + 
				L"\", \"dt\": " + dt + L", \"rowid\":" + rowid + L"}";
		}, (void *)&buffer
	);
	return status;
}

int chatDbReadCb( void* db, std::wstring& activity, unsigned int limit, unsigned int offset, ChatDbReadCallBack cb ) {
	int ret_val = -1;
	if( db == nullptr ) {
		return ret_val;
	}
	sqlite3* _db = (sqlite3 *)db;
	
	std::wstring r;
	create_read_request_string( r, activity, limit, offset );
	int status = make_request( _db, r.c_str(), [](sqlite3_stmt *stmt, void *pvcb ) { 
			std::wstring usr = std::wstring( (wchar_t*)sqlite3_column_text16( stmt, 0 ) );
			std::wstring msg = std::wstring( (wchar_t*)sqlite3_column_text16( stmt, 1 ) );
			unsigned int dt = sqlite3_column_int( stmt, 2 );
			unsigned int rowid = sqlite3_column_int( stmt, 3 );
			ChatDbReadCallBack cb = (ChatDbReadCallBack)pvcb;
			cb( usr, msg, dt, rowid ); 				
		}, (void *)cb
	);
	return status;		
}


int chatDbUpdate( void* db, std::wstring &message, unsigned int rowid ) {
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
	void (*callback)( sqlite3_stmt *statement, void *callback_data), void *callback_data ) 
{
	int ret_val = -1;
	sqlite3_stmt *statement;

	int status = sqlite3_prepare16( _db, request, -1, &statement, NULL );
	if( status == SQLITE_OK ) {
		while((status = sqlite3_step(statement)) == SQLITE_ROW) {
			if( callback != nullptr ) {
				callback( statement, callback_data );
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
	int status = make_request( _db, r.c_str(), [](sqlite3_stmt *stmt, void *cbdata) { 
			*((bool *)cbdata) = (sqlite3_column_int( stmt, 0 ) == 1); 
			return;
		}, (void *)&exists
	);
	return (status == 0 && exists);
}		


static int create_table( sqlite3* _db ) {
	int status;
	std::wstring r = L"CREATE TABLE IF NOT EXISTS " + _chat_table + 
		L" (user TEXT NOT NULL, message TEXT NOT NULL, activity TEXT NOT NULL, datetime INT NOT NULL);";
	status = make_request( _db, r.c_str(), nullptr, nullptr );

	r = L"CREATE INDEX 'datetime_index' ON " + _chat_table + L" (`datetime` DESC)";
	status = make_request( _db, r.c_str(), nullptr, nullptr );

	r = L"CREATE INDEX 'activity_index' ON " + _chat_table + L" (`activity` DESC)";
	status = make_request( _db, r.c_str(), nullptr, nullptr );
	return status;
}


static void create_read_request_string( std::wstring& r, std::wstring& activity, unsigned int limit, unsigned int offset ) {
	r = L"SELECT user, message, datetime, ROWID FROM '" + _chat_table + 
		L"' WHERE activity='" + activity + L"' ORDER BY datetime DESC" + 
		L" LIMIT " + std::to_wstring(limit) + L" OFFSET " + std::to_wstring(offset) + L";";
}