#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <time.h>
#include "chatdb.h"

void read_callback(std::wstring& user, std::wstring& message, unsigned int &datetime, unsigned int& rowid, void *customData) {
	*(int *)(customData) += 1;
	fwprintf( stdout, L"user=%ls, message=%ls, datetime=%u, rowid=%u, customData=%d\n", 
		user.c_str(), message.c_str(), datetime, rowid, *(int *)(customData) );
}

int main( void ) {
	HINSTANCE hDLL;

	hDLL = LoadLibrary("chatdb"); 	// Loading "chatdb.dll" library
	if( hDLL == nullptr ) 	// If failed...
	{
		fprintf( stdout, "Failed to load the .dll. Exiting...");
		return 0;
	}

	fprintf( stdout, "The Dll has been loaded!" );

	// Declaring pointers to the dll functions
	CHAT_DB_OPEN pOpen = (CHAT_DB_OPEN) GetProcAddress (hDLL, "chatDbOpen");
	CHAT_DB_WRITE pWrite = (CHAT_DB_WRITE) GetProcAddress (hDLL, "chatDbWrite");
	CHAT_DB_WRITE_WITH_IMAGE pWriteIm = (CHAT_DB_WRITE_WITH_IMAGE) GetProcAddress (hDLL, "chatDbWriteWithImage");
	CHAT_DB_READ pRead = (CHAT_DB_READ) GetProcAddress (hDLL, "chatDbRead"); 		// This one is for the server
	CHAT_DB_READ_CB pReadCb = (CHAT_DB_READ_CB) GetProcAddress (hDLL, "chatDbReadCb"); 		// This one is for use in SP
	CHAT_DB_UPDATE pUpdate = (CHAT_DB_UPDATE) GetProcAddress (hDLL, "chatDbUpdate");
	CHAT_DB_REMOVE pRemove = (CHAT_DB_REMOVE) GetProcAddress (hDLL, "chatDbRemove");
	CHAT_DB_CLOSE pClose = (CHAT_DB_CLOSE) GetProcAddress (hDLL, "chatDbClose");

	int status;
	void* db;	// A handle for an opened database

	// Opening a database
	db = pOpen( std::wstring(L"chat.db") ); 	// "chat.db" is a database embedded in a single file
	if( db == nullptr ) {	// If failed to open...
		fwprintf( stdout, L"Error opening the \"chat.db\" database. Exiting..." );
		return -1;
	}

	// Generating and writing 10 messages into the database
	for( int i = 0 ; i < 10 ; i++ ) {
		std::wstring act{ L"8" };
		std::wstring usr{ L"user" + std::to_wstring( rand() % 8 ) };
		std::wstring msg{ L"сообщение: " + std::to_wstring( rand() % 50 ) + std::to_wstring( rand() % 50 ) };
		unsigned long rowid;
		unsigned long write_time;
		status = pWrite( db, act, usr, msg, rowid, write_time );
		if( status != 0 ) { 	// "0" - success, "-1" - error
			break;
		}
	}
	if( status != 0 ) {
		fwprintf( stdout, L"Error writing the database. Exiting...\n" );
		return -1;
	}

	fwprintf( stdout, L"\nWriting with image...\n" );
	std::wstring act{ L"8" + std::to_wstring( rand() % 2 ) };
	std::wstring usr{ L"user" + std::to_wstring( rand() % 8 ) };
	std::wstring msg{ L"message: " + std::to_wstring( rand() % 50 ) + std::to_wstring( rand() % 50 ) };
	std::string im{ "1234567890" + std::to_string( rand() % 50 ) + std::to_string( rand() % 50 ) };
	std::string ic{ "1234567890" + std::to_string( rand() % 50 ) + std::to_string( rand() % 50 ) };
	unsigned long rowid;
	unsigned long write_time;
	status = pWriteIm( db, act, usr, msg, ic, im, 10, 10, rowid, write_time );
	fwprintf( stdout, L"\nWrite with image stauts=%d\n", status );

	// Updating a row with ROWID=73
	fwprintf( stdout, L"Updaing rowid=73\n" );
	status = pUpdate( db, std::wstring(L"A NEW MESSAGE"), 73 );
	if( status == 0 ) {	// If no record found it is still "0"
		fprintf( stdout, "Updated successfully.\n");
	} else {
		fprintf( stdout, "Error updating the record!\n");
	}
	
	// Removing a row with ROWID=83
	fwprintf( stdout, L"Removing rowid=83\n" );
	status = pRemove(db, 83);
	if( status == 0 ) { // If no record found it is still "0"
		fprintf( stdout, "Deleted successfully.\n");
	} else {
		fprintf( stdout, "Error deleting the record!\n");
	}

	// Reading all the messages related to "activity0" into a buffer
	std::wstring buffer; 
	fwprintf( stdout, L"Reading:\n" );
	pRead( db, std::wstring( L"8"), 100, 0, -1, buffer );
	fwprintf( stdout, L"%ls\n\n", buffer.c_str() );

	// Reading all the messages related to "activity0" (a callback version)
	fwprintf( stdout, L"Reading with callback:\n" );
	int counter=0;
	pReadCb( db, std::wstring(L"8"), 100, 0, -1, read_callback, &counter);
	fwprintf( stdout, L"Read callback counter value after all readings: %d", counter);
	// Closing the database
	pClose( db );

	FreeLibrary(hDLL);

	return 0;
}