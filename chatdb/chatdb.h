#include <string>
#include <stdexcept>

typedef void (*ChatDbReadCallBack_) (
		char *user, char *message, unsigned int &datetime, unsigned int& rowid, 
		char *icon, unsigned int& imageId, void *customData );		

typedef void (*ChatDbReadImageCallBack_) ( 
		char *image, unsigned int& width, unsigned int& height, void *customData );		

typedef void (*ChatDbActivitiesCallBack_) ( 
	char *activity, unsigned int activity_count, unsigned int updated_at, char *has_new, void *customData );

typedef void* (*CHAT_DB_OPEN_)( const wchar_t *dbFileName );
typedef void (*CHAT_DB_CLOSE_)( void* );

typedef int (*CHAT_DB_WRITE_)( void*, const char *activity, const char *user, const char *message, 
	unsigned long int& rowid, unsigned long int& write_time );

typedef int (*CHAT_DB_WRITE_WITH_IMAGE_)( void*, const char *activity, const char *user, const char *message, 
	const char* icon, const char* image, int width, int height, unsigned long int& rowid, unsigned long int& write_time );

typedef int (*CHAT_DB_READ_)( void*, const char *activity, 
	unsigned int limit, unsigned int offset, unsigned int rowidGreaterThan, ChatDbReadCallBack_ cb, void *customData );

typedef int (*CHAT_DB_READ_ID_)( void* db, unsigned int rowid, ChatDbReadCallBack_ cb, void *customData );

typedef int (*CHAT_DB_READ_IMAGE_)( void* db, unsigned int rowid, ChatDbReadImageCallBack_ cb, void *customData ); 

typedef int (*CHAT_DB_UPDATE_)( void*, const char *message, unsigned int rowid );

typedef int (*CHAT_DB_UPDATE_WITH_IMAGE_)( void*, const char *message, const char *icon, unsigned int rowid, 
	const char *image, unsigned int width, unsigned int height, unsigned int imageid );

typedef int (*CHAT_DB_REMOVE_)( void*, unsigned int rowid );	

typedef int (*CHAT_DB_REMOVE_WITH_IMAGE_)( void*, unsigned int rowid, unsigned int imageid );	

typedef void* (*CHAT_DB_CREATE_IMAGE_)( wchar_t *, int, int, int, int );	
typedef void (*CHAT_DB_FREE_IMAGE_)( void * );	
typedef char* (*CHAT_DB_GET_IMAGE_BUFFER_)( void *, unsigned int&, int&, int & );	
typedef char* (*CHAT_DB_GET_ICON_BUFFER_)( void *, unsigned int& );	
typedef int (*CHAT_DB_GET_IMAGE_STATUS_)( void * );	

// Возвращает (через callback) сколько сообщений отправлено по каждой activity  
typedef int (*CHAT_DB_ACTIVITIES_)( void *db, char *user, ChatDbActivitiesCallBack_ cb, void *customData );	

// Обновляет дату, когда пользователь последний раз читал сообщения, относящиеся к указанной activity
typedef int (*CHAT_DB_UPDATE_USER_READ_)( void* db, char *user, char *activity, unsigned int dt );	


#ifdef CHAT_DB_DLL_EXPORT
	#define CHATDB_DECLSPEC extern "C" __declspec(dllexport)
#else 
	#define CHATDB_DECLSPEC extern "C" __declspec(dllimport)
#endif

CHATDB_DECLSPEC void* chatDbOpen_( const wchar_t *dbFileName ); 	// 0 - success, -1 - error

CHATDB_DECLSPEC void chatDbClose_( void* );

CHATDB_DECLSPEC int chatDbWrite_( void*, const char *activity, const char *user, const char *message, 
	unsigned long int& rowid, unsigned long int& write_time );	// 0 - success, -1 - error

CHATDB_DECLSPEC int chatDbWriteWithImage_( void* db, const char *activity, const char *user, const char *message, 
	const char* icon, const char* image, int width, int height, unsigned long int& rowid, unsigned long int& write_time ); 

CHATDB_DECLSPEC int chatDbRead_( void*, char *activity, 
	unsigned int limit, unsigned int offset, unsigned int rowidGreaterThan, ChatDbReadCallBack_ cb, void *customData );

CHATDB_DECLSPEC int chatDbReadImage_( void* db,  unsigned int rowid, ChatDbReadImageCallBack_ cb, void *customData ); 

CHATDB_DECLSPEC int chatDbUpdate_( void*, const char *message, unsigned int rowid );	// 0 - success, -1 - error

CHATDB_DECLSPEC int chatDbUpdateWithImage_( void*, const char *message, const char *icon, unsigned int rowid, 
	const char *image, unsigned int width, unsigned int height, unsigned int imageid );	// 0 - success, -1 - error

CHATDB_DECLSPEC int chatDbRemove_( void*, unsigned int rowid );	// 0 - success, -1 - error

CHATDB_DECLSPEC int chatDbRemoveWithImage_( void*, unsigned int rowid, unsigned int imageid );	// 0 - success, -1 - error

CHATDB_DECLSPEC int chatDbReadId_( void* db, unsigned int rowid, ChatDbReadCallBack_ cb, void *customData );

CHATDB_DECLSPEC void *chatDbCreateImage_( wchar_t *src, int, int, int, int );

CHATDB_DECLSPEC void chatDbFreeImage_( void *im );

CHATDB_DECLSPEC char *chatDbGetImageBuffer_( void *im, unsigned int& length, int& width, int& height );

CHATDB_DECLSPEC char *chatDbGetIconBuffer_( void *im, unsigned int& length );

CHATDB_DECLSPEC int chatDbGetImageStatus_( void *im );

CHATDB_DECLSPEC int chatDbActivities_( void* db,  char *user, ChatDbActivitiesCallBack_ cb, void *customData );

CHATDB_DECLSPEC int chatDbUpdateUserRead_( void* db, char *user, char *activity, unsigned int dt );