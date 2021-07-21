#include <string>
#include <stdexcept>

typedef void (*ChatDbReadCallBack_) (
		char *user, char *message, unsigned int &datetime, unsigned int& rowid, 
		char *icon, unsigned int& imageId, void *customData );		

typedef void (*ChatDbReadImageCallBack_) ( 
		char *image, unsigned int& width, unsigned int& height, void *customData );		

typedef void* (*CHAT_DB_OPEN_)( const wchar_t *dbFileName );
typedef void (*CHAT_DB_CLOSE_)( void* );

typedef int (*CHAT_DB_WRITE_)( void*, const char *activity, const char *user, const char *message, 
	unsigned long int& rowid, unsigned long int& write_time );

typedef int (*CHAT_DB_WRITE_IMAGE_)( void*, const char *activity, const char *user, const char *message, 
	const char* icon, const char* image, int width, int height, unsigned long int& rowid, unsigned long int& write_time );

typedef int (*CHAT_DB_READ_)( void*, const char *activity, 
	unsigned int limit, unsigned int offset, unsigned int rowidGreaterThan, ChatDbReadCallBack_ cb, void *customData );

typedef int (*CHAT_DB_READ_IMAGE_)( void* db, unsigned int rowid, ChatDbReadImageCallBack_ cb, void *customData ); 

typedef int (*CHAT_DB_UPDATE_)( void*, const char *message, unsigned int rowid );
typedef int (*CHAT_DB_REMOVE_)( void*, unsigned int rowid );	

#ifdef CHAT_DB_DLL_EXPORT
	#define CHATDB_DECLSPEC extern "C" __declspec(dllexport)
#else 
	#define CHATDB_DECLSPEC extern "C" __declspec(dllimport)
#endif

CHATDB_DECLSPEC void* chatDbOpen_( const wchar_t *dbFileName ); 	// 0 - success, -1 - error

CHATDB_DECLSPEC void chatDbClose_( void* );

CHATDB_DECLSPEC int chatDbWrite_( void*, const char *activity, const char *user, const char *message, 
	unsigned long int& rowid, unsigned long int& write_time );	// 0 - success, -1 - error

CHATDB_DECLSPEC int chatDbWriteImage_( void* db, const char *activity, const char *user, const char *message, 
	const char* icon, const char* image, int width, int height, unsigned long int& rowid, unsigned long int& write_time ); 

CHATDB_DECLSPEC int chatDbRead_( void*, char *activity, 
	unsigned int limit, unsigned int offset, unsigned int rowidGreaterThan, ChatDbReadCallBack_ cb, void *customData );

CHATDB_DECLSPEC int chatDbReadImage_( void* db,  unsigned int rowid, ChatDbReadImageCallBack_ cb, void *customData ); 

CHATDB_DECLSPEC int chatDbUpdate_( void*, const char *message, unsigned int rowid );	// 0 - success, -1 - error

CHATDB_DECLSPEC int chatDbRemove_( void*, unsigned int rowid );	// 0 - success, -1 - error
