#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <time.h>
#include "chatdb.h"

FILE *_logfile = nullptr;

void read_callback(char *user, char *message, unsigned int &datetime, unsigned int& rowid, 
	char *icon, unsigned int& imageId, void *customData) 
{
	*(int *)(customData) += 1;
	fprintf( _logfile, "user=%s, message=%s, datetime=%u, rowid=%u, icon=%s, imageId=%u, customData=%d\n", 
		user, message, datetime, rowid, icon, imageId, *(int *)(customData) );
}

void read_id_callback(char *user, char *message, unsigned int &datetime, unsigned int& rowid, 
	char *icon, unsigned int& imageId, void *customData) 
{
	*(int *)(customData) += 1;
	fprintf( _logfile, "user=%s, message=%s, datetime=%u, rowid=%u, icon=%s, imageId=%u, customData=%d\n", 
		user, message, datetime, rowid, icon, imageId, *(int *)(customData) );
}

void read_image_callback(char *image, unsigned int& width, unsigned int& height, void *customData ) 
{
	fprintf( _logfile, "image=%s, width=%u, height=%d, %d\n", image, width, height, *(int*)customData );
}

// To read how many chat messages each activity has...
void activities_callback( char *activity, unsigned int count, unsigned int updated_at, char *has_new, void *customData ) 
{
	*(int *)(customData) += 1;
	fprintf( _logfile, "activity=%s, count=%u, updated_at=%u, has_new=%s\n", 
		activity, count, updated_at, has_new, *(int *)(customData) );
}

int WINAPI WinMain(
   HINSTANCE hInstance,
   HINSTANCE hPrevInstance,
   LPSTR     lpCmdLine,
   int       nCmdShow
)
{
	_logfile = fopen("chatdb.log", "w");

	HINSTANCE hDLL;

	hDLL = LoadLibrary("chatdb"); 	// Loading "chatdb.dl" library
	if( hDLL == nullptr ) 	// If failed...
	{
		fprintf( _logfile, "Failed to load the .dll. Exiting...");
		return 0;
	}

	fprintf( _logfile, "The Dll has been loaded!" );

	// Declaring pointers to the dll functions
	CHAT_DB_OPEN_ pOpen = (CHAT_DB_OPEN_) GetProcAddress (hDLL, "chatDbOpen_");
	CHAT_DB_WRITE_ pWrite = (CHAT_DB_WRITE_) GetProcAddress (hDLL, "chatDbWrite_");
	CHAT_DB_WRITE_WITH_IMAGE_ pWriteWithImage = (CHAT_DB_WRITE_WITH_IMAGE_) GetProcAddress (hDLL, "chatDbWriteWithImage_");
	CHAT_DB_READ_ pRead = (CHAT_DB_READ_) GetProcAddress (hDLL, "chatDbRead_"); 		
	CHAT_DB_READ_ID_ pReadId = (CHAT_DB_READ_ID_) GetProcAddress (hDLL, "chatDbReadId_"); 		
	CHAT_DB_READ_IMAGE_ pReadImage = (CHAT_DB_READ_IMAGE_) GetProcAddress (hDLL, "chatDbReadImage_"); 
	CHAT_DB_UPDATE_ pUpdate = (CHAT_DB_UPDATE_) GetProcAddress (hDLL, "chatDbUpdate_");
	CHAT_DB_UPDATE_WITH_IMAGE_ pUpdateWithImage = (CHAT_DB_UPDATE_WITH_IMAGE_) GetProcAddress (hDLL, "chatDbUpdateWithImage_");
	CHAT_DB_REMOVE_ pRemove = (CHAT_DB_REMOVE_) GetProcAddress (hDLL, "chatDbRemove_");
	CHAT_DB_REMOVE_WITH_IMAGE_ pRemoveWithImage = (CHAT_DB_REMOVE_WITH_IMAGE_) GetProcAddress (hDLL, "chatDbRemoveWithImage_");
	CHAT_DB_CLOSE_ pClose = (CHAT_DB_CLOSE_) GetProcAddress (hDLL, "chatDbClose_");

	CHAT_DB_CREATE_IMAGE_ pCreateImage = (CHAT_DB_CREATE_IMAGE_) GetProcAddress (hDLL, "chatDbCreateImage_"); 
	CHAT_DB_FREE_IMAGE_ pFreeImage = (CHAT_DB_FREE_IMAGE_) GetProcAddress (hDLL, "chatDbFreeImage_"); 
	CHAT_DB_GET_IMAGE_BUFFER_ pGetImageBuffer = (CHAT_DB_GET_IMAGE_BUFFER_) GetProcAddress (hDLL, "chatDbGetImageBuffer_"); 
	CHAT_DB_GET_ICON_BUFFER_ pGetIconBuffer = (CHAT_DB_GET_ICON_BUFFER_) GetProcAddress (hDLL, "chatDbGetIconBuffer_"); 
	CHAT_DB_GET_IMAGE_STATUS_ pGetImageStatus = (CHAT_DB_GET_IMAGE_STATUS_) GetProcAddress (hDLL, "chatDbGetImageStatus_"); 

	CHAT_DB_ACTIVITIES_ pActivities = (CHAT_DB_ACTIVITIES_) GetProcAddress (hDLL, "chatDbActivities_"); 
	CHAT_DB_UPDATE_USER_READ_ pUpdateUserRead = (CHAT_DB_UPDATE_USER_READ_) GetProcAddress (hDLL, "chatDbUpdateUserRead_"); 

	int status;
	void* db;	// A handle for an opened database

	// Opening a database
	db = pOpen( L"chat.db" ); 	// "chat.db" is a database embedded in a single file
	if( db == nullptr ) {	// If failed to open...
		fprintf( _logfile, "Error opening the \"chat.db\" database. Exiting..." );
		return -1;
	}

	for( int i = 0 ; i < 4 ; i++ ) {
		fprintf( _logfile, "\nWriting with image...\n" );
		std::string act{ "8" };
		std::string usr{ "user" + std::to_string( rand() % 8 ) };
		std::string msg{ "message: " + std::to_string( rand() % 50 ) + std::to_string( rand() % 50 ) };
		std::string im{ "1234567890" + std::to_string( rand() % 50 ) + std::to_string( rand() % 50 ) };
		std::string ic{ "1234567890" + std::to_string( rand() % 50 ) + std::to_string( rand() % 50 ) };
		unsigned long rowid;
		unsigned long write_time;
		status = pWriteWithImage( db, act.c_str(), usr.c_str(), msg.c_str(), ic.c_str(), im.c_str(), 10, 10, rowid, write_time );
		fprintf( _logfile, "\nWrite with image status=%d\n", status );
		if( status != 0 ) { 	// "0" - success, "-1" - error
			return -1;
		}
	}

	for( int i = 0 ; i < 4 ; i++ ) {
		std::string act{ "8" };
		std::string usr{ "user" + std::to_string( rand() % 8 ) };
		std::string msg{ "сообщение: " + std::to_string( rand() % 50 ) + std::to_string( rand() % 50 ) };
		unsigned long rowid;
		unsigned long write_time;
		status = pWrite( db, act.c_str(), usr.c_str(), msg.c_str(), rowid, write_time );
		fprintf( _logfile, "\nWrite status=%d\n", status );
		if( status != 0 ) { 	// "0" - success, "-1" - error
			return -1;
		}
	}

	// Reading all the messages related to activity "8" (a callback version)
	fprintf( _logfile, "Reading with a callback:\n" );
	int counter=0;
	pRead( db, "8", 100, 0, -1, read_callback, &counter);
	fprintf( _logfile, "Read callback counter value after all readings: %d\n", counter);
	// Closing the database

	// Reading an image with a callback
	fprintf( _logfile, "Reading an image with a callback:\n" );
	int imageId=3;
	pReadImage( db, imageId, read_image_callback, &imageId );
	// Closing the database


	// Updating a row with ROWID=4
	fprintf( _logfile, "Updaing rowid=5\n" );
	status = pUpdate( db, "AN UPDATE!!!!! MESSAGE", 5 );
	if( status == 0 ) {	// If no record found it is still "0"
		fprintf( _logfile, "Updated successfully.\n");
	} else {
		fprintf( _logfile, "Error updating the record!\n");
	}
	
	// Removing a row with ROWID=3
	fprintf( _logfile, "Removing rowid=83\n" );
	status = pRemove(db, 83);
	if( status == 0 ) { // If no record found it is still "0"
		fprintf( _logfile, "Deleted successfully.\n");
	} else {
		fprintf( _logfile, "Error deleting the record!\n");
	}
	// Updating a row with ROWID=1
	// Обновляем сообщение и изображение
	fprintf( _logfile, "Updating rowid=1, imageId=1, with image read\n" );
	// Читаем и форматируем изображение, потом пишем в базу
	void *im = pCreateImage(L"test.jpg", 1200, 900, 200, 150);
	if( pGetImageStatus(im) == 0 ) { 	// '0' stands for Ok
		unsigned int imlen;
		int imw, imh;
		char *im_buffer = pGetImageBuffer(im, imlen, imw, imh);
		unsigned int iclen;
		char *ic_buffer = pGetIconBuffer(im, iclen);	

		// Тут тебе надо перекодивать im_buffer и ic_buffer в base64 - там символы "кривые", в базу не запишутся. Я мусором заполняю и пишу в базу
		for( int i = 0 ; i < iclen-1 ; i++ ) {
			ic_buffer[i] = '0';	
		}
		ic_buffer[iclen-1] = '\x0'; 
		for( int i = 0 ; i < imlen-1 ; i++ ) {
			im_buffer[i] = '0';	
		}
		im_buffer[imlen-1] = '\x0'; 
		status = pUpdateWithImage(db, "A NEW MESSAGE!!!!!", ic_buffer, 1, im_buffer, imw, imh, 1);
	}
	pFreeImage(im);

	if( status == 0 ) { // If no record found it is still "0"
		fprintf( _logfile, "Updated successfully.\n");
	} else {
		fprintf( _logfile, "Error updating the record!\n");
	}

	// Updating a row with ROWID=5
	// Обновляем сообщение, добавляя новое изображение
	fprintf( _logfile, "Updating and adding an image: rowid=5, imageId=0\n" );
	status = pUpdateWithImage(db, "A NEW MESSAGE!!!!!", "A NEW ICON", 5, "A NEW IMAGE", 400, 400, 0);
	if( status == 0 ) { // If no record found it is still "0"
		fprintf( _logfile, "Updated successfully.\n");
	} else {
		fprintf( _logfile, "Error updating the record!\n");
	}

// Updating a row with ROWID=2 - deleting an image attached
// Удаляем изображение, но обновляем сообщение: передаем пустые строки в icon и image
	fprintf( _logfile, "Updating rowid=2, imageId=2 - deleting an image attached\n" );
	status = pUpdateWithImage(db, "A NEW MESSAGE!!!!!", "", 2, "", 400, 400, 2);
	if( status == 0 ) { // If no record found it is still "0"
		fprintf( _logfile, "Updated successfully.\n");
	} else {
		fprintf( _logfile, "Error updating the record!\n");
	}

	// Reading all the messages related to activity "8" (a callback version)
	fprintf( _logfile, "Reading with a callback:\n" );
	counter=0;
	pRead( db, "8", 100, 0, -1, read_callback, &counter);
	fprintf( _logfile, "Read callback counter value after all readings: %d\n", counter);

	// Reading a message by id (a callback version)
	fprintf( _logfile, "Reading a message by id=3:\n" );
	int read_id_user_data=0;
	pReadId( db, 3, read_callback, &read_id_user_data);
	fprintf( _logfile, "Read by id callback custom value: %d\n", read_id_user_data);
	// Closing the database

	fprintf( _logfile, "Reading how many chat messages each activity has:\n" );
	int activities_user_data = 0; 	// custom data
	pActivities( db, "user1", activities_callback, &activities_user_data );
	fprintf( _logfile, "Activities callback custom value after all readings: %d\n", activities_user_data);

	fprintf( _logfile, "Updating last user read:\n" );
	pUpdateUserRead( db, "user1", "8", 162687777 );

	// Closing the databas
	pClose( db );

	FreeLibrary(hDLL);

	fclose(_logfile);

	return 0;
}