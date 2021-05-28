#include <string>
//#include <iostream>
//#include <thread>
//#include "Globals.hpp"
#include <iostream>
#include <windows.h>
#include "WebServer.hpp"
#include "chatdb.h"

using namespace std;

#ifndef UNICODE
#define UNICODE
#endif 

// A callback to read from databases
void read_chat_db_callback(std::wstring& user, std::wstring& message, unsigned int &datetime, unsigned int& rowid) {
	fwprintf( stderr, L"user=%ls, message=%ls, datetime=%u, rowid=%u\n", user.c_str(), message.c_str(), datetime, rowid );
}

// The pointers to the functions for database management
CHAT_DB_OPEN pOpen;
CHAT_DB_WRITE pWrite;
CHAT_DB_READ pRead;
CHAT_DB_READ_CB pReadCb;
CHAT_DB_UPDATE pUpdate;
CHAT_DB_REMOVE pRemove;
CHAT_DB_CLOSE pClose;

int callback ( ServerData *sd ); 	// A callback for the server

static SERVER_DLL_START p_server_start;

static StartServerData Data;

//int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow)
int main( void )
{
    HINSTANCE hServerDLL;
		HINSTANCE hChatDbDLL;

		hChatDbDLL = LoadLibrary("chatdb"); 	// Loading "chatdb.dll" library
		if( hChatDbDLL == nullptr ) 	// If failed...
		{
			cerr << "Failed to load the chat .dll. Exiting...";
			return 0;
		}
		cerr << "The chat Dll has been loaded!";
		pOpen = (CHAT_DB_OPEN) GetProcAddress (hChatDbDLL, "chatDbOpen");
		pWrite = (CHAT_DB_WRITE) GetProcAddress (hChatDbDLL, "chatDbWrite");
		pRead = (CHAT_DB_READ) GetProcAddress (hChatDbDLL, "chatDbRead"); 		// This one is for the server
		pReadCb = (CHAT_DB_READ_CB) GetProcAddress (hChatDbDLL, "chatDbReadCb"); 		// This one is for use in SP
		pUpdate = (CHAT_DB_UPDATE) GetProcAddress (hChatDbDLL, "chatDbUpdate");
		pRemove = (CHAT_DB_REMOVE) GetProcAddress (hChatDbDLL, "chatDbRemove");
		pClose = (CHAT_DB_CLOSE) GetProcAddress (hChatDbDLL, "chatDbClose");

    Data.IP = "127.0.0.1";
    Data.Port = "8010";
    Data.ExePath = nullptr;
    Data.HtmlPath = "html\\";

    hServerDLL = LoadLibrary ("serverweb");
    if (hServerDLL != NULL)
    {
        std::cout << "Starting!" << std::endl;
        Data.Message = ssd_Start;
        p_server_start = (SERVER_DLL_START) GetProcAddress (hServerDLL, "start");

        if (p_server_start != NULL) {
            //int (*callback_ptr)(ServerData *) = callback;
            //cerr << "Server is about to start!" << endl;

            p_server_start (&Data, callback);
            //MessageBoxW( NULL, L"Press \"STOP\" to stop the server...", L"SP-Server", MB_OK );
            cerr << "The server has started! Press <ENTER> to stop the server..."  << endl;
            cin.get();
            Data.Message = ssd_Stop;
            p_server_start (&Data, callback);
            cerr << "The server is stopped! Press <ENTER> to exit the program..."  << endl;
            cin.get();
    } else {
      cerr << "The server has not started!" << endl;
    }
    FreeLibrary(hServerDLL);
  }

	FreeLibrary(hChatDbDLL);
  return 0;
}


char _callback_error_code;
#define RESPONSE_BUFFER 100000
char _callback_response[RESPONSE_BUFFER+1];

int callback ( ServerData *sd ) {
	int status;
	void* db;	// A handle for an opened database

  _callback_error_code = 0;
  sd->sp_response_buf = _callback_response;
  sd->sp_free_response_buf = false;
  sd->sp_response_is_file = false;

	bool success = false;
  if( sd->message_id == SERVER_CHAT_READ ) {
		// {"sessId":"ABCDEFGH","user":"user0","projectId":"1234567890","activity":"activityCode"}
		db = pOpen( std::wstring(L"chat.db") ); 	
		if( db != nullptr ) {	// If opened...
			std::wstring buffer; 
			pRead( db, std::wstring( L"activity0"), 100, 0, buffer );
			sprintf( sd->sp_response_buf, "{ \"errcode\": 0, \"error\": \"\", \"buffer\": [ %ls ] }", buffer.c_str() );
			sd->sp_response_buf_size = strlen(sd->sp_response_buf);
			_callback_error_code = 0;
			success = true;
			pClose( db );
		}
  }
  else if( sd->message_id == SERVER_CHAT_WRITE ) {
		// {"sessId":"ABCDEFGH","user":"user0","projectId":"1234567890","activity":"activityCode","message":"a new message"}: 
		db = pOpen( std::wstring(L"chat.db") ); 	
		if( db != nullptr ) {	// If opened...
			std::wstring buffer; 
			std::wstring act = L"activity0";
			std::wstring usr = L"user0";
			std::wstring msg = L"a message from user 0 sent through the dll";
			unsigned long int rowid, write_time;
			status = pWrite( db, act, usr, msg, rowid, write_time );
			if( !(status < 0) ) {
				sprintf( sd->sp_response_buf, "{ \"errcode\": 0, \"error\": \"\", \"rowid\": %d, \"dt\": %d }", rowid, write_time );
				sd->sp_response_buf_size = strlen(sd->sp_response_buf);
				_callback_error_code = 0;
				success = true;
			}
			pClose( db );
		}
  }
  else if( sd->message_id == SERVER_CHAT_UPDATE ) {
		// {"sessId":"ABCDEFGH","user":"user0","projectId":"1234567890","rowid":114,"message":"an updated message"}
    db = pOpen( std::wstring(L"chat.db") ); 	
		if( db != nullptr ) {	// If opened...
			std::wstring msg = L"a message update from user 0 sent through the dll";
			status = pUpdate( db, msg, 73 );
			if( !(status < 0) ) {
				sprintf( sd->sp_response_buf, "{ \"errcode\": 0, \"error\": \"\" }" );
				sd->sp_response_buf_size = strlen(sd->sp_response_buf);
				_callback_error_code = 0;
				success = true;
			}
			pClose( db );
		}
  }
  else if( sd->message_id == SERVER_CHAT_REMOVE ) {
		// {"sessId":"ABCDEFGH","user":"user0","projectId":"1234567890","rowid":114}
    db = pOpen( std::wstring(L"chat.db") ); 	
		if( db != nullptr ) {	// If opened...
			status = pRemove( db, 83 );
			if( !(status < 0) ) {
				sprintf( sd->sp_response_buf, "{ \"errcode\": 0, \"error\": \"\" }" );
				sd->sp_response_buf_size = strlen(sd->sp_response_buf);
				_callback_error_code = 0;
				success = true;
			}
			pClose( db );
		}
  }
	
  if( !success ) {
    sd->sp_response_buf[0] = '\x0';
    sd->sp_response_buf_size = 0;
    _callback_error_code = -1;
  }
  return _callback_error_code;
}

