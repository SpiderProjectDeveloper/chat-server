#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <time.h>
#include "chatdb.h"

void read_callback(char *user, char *message, unsigned int &datetime, unsigned int& rowid, 
	char *icon, unsigned int& imageId, void *customData) 
{
	*(int *)(customData) += 1;
	fprintf( stdout, "user=%s, message=%s, datetime=%u, rowid=%u, icon=%s, inageId=%u, customData=%d\n", 
		user, message, datetime, rowid, icon, imageId, *(int *)(customData) );
}

void read_image_callback(char *image, unsigned int& width, unsigned int& height, void *customData ) 
{
	fprintf( stdout, "image=%s, width=%u, height=%d, %d\n", image, width, height, *(int*)customData );
}


int main( void ) {
	HINSTANCE hDLL;

	hDLL = LoadLibrary("chatdb"); 	// Loading "chatdb.dl" library
	if( hDLL == nullptr ) 	// If failed...
	{
		fprintf( stdout, "Failed to load the .dll. Exiting...");
		return 0;
	}

	fprintf( stdout, "The Dll has been loaded!" );

	// Declaring pointers to the dll functions
	CHAT_DB_OPEN_ pOpen = (CHAT_DB_OPEN_) GetProcAddress (hDLL, "chatDbOpen_");
	CHAT_DB_WRITE_ pWrite = (CHAT_DB_WRITE_) GetProcAddress (hDLL, "chatDbWrite_");
	CHAT_DB_WRITE_IMAGE_ pWriteIm = (CHAT_DB_WRITE_IMAGE_) GetProcAddress (hDLL, "chatDbWriteImage_");
	CHAT_DB_READ_CB_ pReadCb = (CHAT_DB_READ_CB_) GetProcAddress (hDLL, "chatDbReadCb_"); 		// This one is for use in SP
	CHAT_DB_READ_IMAGE_CB_ pReadImageCb = (CHAT_DB_READ_IMAGE_CB_) GetProcAddress (hDLL, "chatDbReadImageCb_"); 		// This one is for use in SP
	CHAT_DB_UPDATE_ pUpdate = (CHAT_DB_UPDATE_) GetProcAddress (hDLL, "chatDbUpdate_");
	CHAT_DB_REMOVE_ pRemove = (CHAT_DB_REMOVE_) GetProcAddress (hDLL, "chatDbRemove_");
	CHAT_DB_CLOSE_ pClose = (CHAT_DB_CLOSE_) GetProcAddress (hDLL, "chatDbClose_");

	int status;
	void* db;	// A handle for an opened database

	// Opening a database
	db = pOpen( "chat.db" ); 	// "chat.db" is a database embedded in a single file
	if( db == nullptr ) {	// If failed to open...
		fprintf( stdout, "Error opening the \"chat.db\" database. Exiting..." );
		return -1;
	}

	fprintf( stdout, "\nWriting with image...\n" );
	std::string act{ "8" };
	std::string usr{ "user" + std::to_string( rand() % 8 ) };
	std::string msg{ "message: " + std::to_string( rand() % 50 ) + std::to_string( rand() % 50 ) };
	std::string im{ "1234567890" + std::to_string( rand() % 50 ) + std::to_string( rand() % 50 ) };
	std::string ic{ "1234567890" + std::to_string( rand() % 50 ) + std::to_string( rand() % 50 ) };
	unsigned long rowid;
	unsigned long write_time;
	status = pWriteIm( db, act.c_str(), usr.c_str(), msg.c_str(), ic.c_str(), im.c_str(), 10, 10, rowid, write_time );
	fprintf( stdout, "\nWrite with image stauts=%d\n", status );

	// Generating and writing 2 messages into the database
	for( int i = 0 ; i < 2 ; i++ ) {
		std::string act{ "8" };
		std::string usr{ "user" + std::to_string( rand() % 8 ) };
		std::string msg{ "сообщение: " + std::to_string( rand() % 50 ) + std::to_string( rand() % 50 ) };
		unsigned long rowid;
		unsigned long write_time;
		status = pWrite( db, act.c_str(), usr.c_str(), msg.c_str(), rowid, write_time );
		if( status != 0 ) { 	// "0" - success, "-1" - error
			break;
		}
	}
	if( status != 0 ) {
		fprintf( stdout, "Error writing the database. Exiting...\n" );
		return -1;
	}

	// Updating a row with ROWID=73
	fprintf( stdout, "Updaing rowid=73\n" );
	status = pUpdate( db, "A NEW MESSAGE", 73 );
	if( status == 0 ) {	// If no record found it is still "0"
		fprintf( stdout, "Updated successfully.\n");
	} else {
		fprintf( stdout, "Error updating the record!\n");
	}
	
	// Removing a row with ROWID=83
	fprintf( stdout, "Removing rowid=83\n" );
	status = pRemove(db, 83);
	if( status == 0 ) { // If no record found it is still "0"
		fprintf( stdout, "Deleted successfully.\n");
	} else {
		fprintf( stdout, "Error deleting the record!\n");
	}

	// Reading all the messages related to "activity0" (a callback version)
	fprintf( stdout, "Reading with a callback:\n" );
	int counter=0;
	pReadCb( db, "8", 100, 0, -1, read_callback, &counter);
	fprintf( stdout, "Read callback counter value after all readings: %d\n", counter);
	// Closing the database

	// Reading an image with a callback
	fprintf( stdout, "Reading an image with a callback:\n" );
	int imageId=3;
	pReadImageCb( db, imageId, read_image_callback, &imageId );
	// Closing the database


	pClose( db );

	FreeLibrary(hDLL);

	return 0;
}