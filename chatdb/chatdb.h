#include <string>
#include <stdexcept>

typedef void (*ChatDbReadCallBack)(std::wstring& user, std::wstring& message, unsigned int &datetime, unsigned int& rowid);		

typedef void* (*CHAT_DB_OPEN)( std::wstring& dbFileName );
typedef void (*CHAT_DB_CLOSE)( void* );
typedef int (*CHAT_DB_WRITE)( void*, std::wstring& activity, std::wstring& user, std::wstring& message, 
	unsigned long int& rowid, unsigned long int& write_time );
typedef int (*CHAT_DB_READ)( void*, std::wstring& activity, unsigned int limit, unsigned int offset, std::wstring& buffer );
typedef int (*CHAT_DB_READ_CB)( void*, std::wstring& activity, unsigned int limit, unsigned int offset, ChatDbReadCallBack cb );
typedef int (*CHAT_DB_UPDATE)( void*, std::wstring& message, unsigned int rowid );
typedef int (*CHAT_DB_REMOVE)( void*, unsigned int rowid );	

#ifdef CHAT_DB_DLL_EXPORT
	#define CHATDB_DECLSPEC extern "C" __declspec(dllexport)
#else 
	#define CHATDB_DECLSPEC extern "C" __declspec(dllimport)
#endif

CHATDB_DECLSPEC void* chatDbOpen( std::wstring& dbFileName ); 	// 0 - success, -1 - error

CHATDB_DECLSPEC void chatDbClose( void* );

CHATDB_DECLSPEC int chatDbWrite( void*, std::wstring& activity, std::wstring& user, std::wstring& message, 
	unsigned long int& rowid, unsigned long int& write_time );	// 0 - success, -1 - error

CHATDB_DECLSPEC int chatDbRead( void*, std::wstring& activity, 
	unsigned int limit, unsigned int offset, std::wstring& buffer );	// 0 - success, -1 - error

CHATDB_DECLSPEC int chatDbReadCb( void*, std::wstring& activity, 
	unsigned int limit, unsigned int offset, ChatDbReadCallBack cb );

CHATDB_DECLSPEC int chatDbUpdate( void*, std::wstring& message, unsigned int rowid );	// 0 - success, -1 - error

CHATDB_DECLSPEC int chatDbRemove( void*, unsigned int rowid );	// 0 - success, -1 - error
