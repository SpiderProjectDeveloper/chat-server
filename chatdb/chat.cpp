#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <time.h>
#include "chatdb.h"

void read_callback(std::wstring& user, std::wstring& message, unsigned int &datetime, unsigned int& rowid) {
	fwprintf( stderr, L"user=%ls, message=%ls, datetime=%u, rowid=%u\n", user.c_str(), message.c_str(), datetime, rowid );
}

int main( void ) {
	HINSTANCE hDLL;

	hDLL = LoadLibrary("chatdb"); 	// Loading "chatdb.dll" library
	if( hDLL == nullptr ) 	// If failed...
	{
		fprintf( stderr, "Failed to load the .dll. Exiting...");
		return 0;
	}

	fprintf( stderr, "The Dll has been loaded!" );

	// Declaring pointers to the dll functions
	CHAT_DB_OPEN pOpen = (CHAT_DB_OPEN) GetProcAddress (hDLL, "chatDbOpen");
	CHAT_DB_WRITE pWrite = (CHAT_DB_WRITE) GetProcAddress (hDLL, "chatDbWrite");
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
		fwprintf( stderr, L"Error opening the \"chat.db\" database. Exiting..." );
		return -1;
	}

	// Generating and writing 10 messages into the database
	for( int i = 0 ; i < 10 ; i++ ) {
		std::wstring act{ L"activity" + std::to_wstring( rand() % 2 ) };
		std::wstring usr{ L"user" + std::to_wstring( rand() % 8 ) };
		std::wstring msg{ L"a message with a random number: " + std::to_wstring( rand() % 50 ) + std::to_wstring( rand() % 50 ) };

		status = pWrite( db, act, usr, msg );
		if( status != 0 ) { 	// "0" - success, "-1" - error
			break;
		}
	}
	if( status != 0 ) {
		fwprintf( stderr, L"Error writing the database. Exiting...\n" );
		return -1;
	}

	// Updating a row with ROWID=73
	fwprintf( stderr, L"Updaing rowid=73\n" );
	status = pUpdate( db, std::wstring(L"A NEW MESSAGE"), 73 );
	if( status == 0 ) {	// If no record found it is still "0"
		fprintf( stderr, "Updated successfully.\n");
	} else {
		fprintf( stderr, "Error updating the record!\n");
	}
	
	// Removing a row with ROWID=83
	fwprintf( stderr, L"Removing rowid=83\n" );
	status = pRemove(db, 83);
	if( status == 0 ) { // If no record found it is still "0"
		fprintf( stderr, "Deleted successfully.\n");
	} else {
		fprintf( stderr, "Error deleting the record!\n");
	}

	// Reading all the messages related to "activity0" into a buffer
	std::wstring buffer; 
	fwprintf( stderr, L"Reading:\n" );
	pRead( db, std::wstring( L"activity0"), 100, 0, buffer );
	fwprintf( stderr, L"%ls\n\n", buffer.c_str() );

	// Reading all the messages related to "activity0" (a callback version)
	fwprintf( stderr, L"Reading with callback:\n" );
	pReadCb( db, std::wstring(L"activity0"), 100, 0, read_callback );

	// Closing the database
	pClose( db );

	FreeLibrary(hDLL);

	return 0;
}