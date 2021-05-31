#include <string>
#include <iostream>
#include <windows.h>
#include "ChatServer.hpp"
#include "chatdb.h"

using namespace std;

#ifndef UNICODE
#define UNICODE
#endif 

// A callback to read from databases
void read_chat_db_callback(std::wstring& user, std::wstring& message, unsigned int &datetime, unsigned int& rowid) {

	// Тут я просто вывожу на stderr прочитанные значения. 
	fwprintf( stderr, L"user=%ls, message=%ls, datetime=%u, rowid=%u\n", user.c_str(), message.c_str(), datetime, rowid );

	// На самом деле здесь надо реализовать функционал, который покажет пользователю за десктопом содержимое чата.	
	// Вызов этого callback'а инициируется другим вызовом:
	// 		pReadCb( 
	//				db, 												// void *,  указатель на базу 
	//				activityCode, 							// std::wstring&, 
	//				limit,											// unsigned int, сколько записей прочитать 
	// 				offset, 										// unsigned int, начиная с какой читать -  
	// 				read_chat_db_callback ); 		// Собственно callback
	//     
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
	HINSTANCE hChatDbDLL, hChatServerDLL;

	hChatDbDLL = LoadLibrary("chatdb"); 	// Loading the "chatdb.dll" library
	if( hChatDbDLL != nullptr ) { 		// If loaded Ok...
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

		// Loading the "chat server" .dll
		hChatServerDLL = LoadLibrary ("serverchat");
		if (hChatServerDLL != NULL) 	// If loaded Ok...
		{
			cerr << "Starting!" << endl;
			Data.Message = ssd_Start;
			p_server_start = (SERVER_DLL_START) GetProcAddress (hChatServerDLL, "start");

			if (p_server_start != NULL) {
					p_server_start (&Data, callback);
					cerr << "The server has started! Press <ENTER> to stop the server..."  << endl;
					cin.get();
					Data.Message = ssd_Stop;
					p_server_start (&Data, callback);
					cerr << "The server is stopped! Press <ENTER> to exit the program..."  << endl;
					cin.get();
			} else {
				cerr << "The server has not started!" << endl;
			}
			FreeLibrary(hChatServerDLL);
		} else {
			cerr << "Failed to load the server .dll. Exiting..." << endl;
		}

		FreeLibrary(hChatDbDLL);
	} else {
		cerr << "Failed to load the chat db .dll. Exiting..." << endl;
	}
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
		// Читаем все сообщения, относящиеся к указанному проекту (projectId) и activity (activityCode).
		// Приходит:
		// {"sessId":"ABCDEFGH","user":"user0","projectId":"1234567890","activity":"activityCode", "limit":100, "offset":0 }
		// Надо: 1) проверить sessId, 2) открыть (pOpen) базу с чатами по проекту (имя файла базы привязано к проекту),
		// 3) сделать запрос "pRead" по указанной "activity".
		// *** Если ключ "limit" отсутствует, надо взять 100. Если ключ "offset" отсутствует, надо взять 0.

		std::wstring dbFileName{L"chat.db"}; 	// Надо прочитать из свойств проекта (проект найти по projectId)
		db = pOpen( dbFileName ); 	
		if( db != nullptr ) {	// If opened...
			std::wstring buffer; 
			std::wstring act = L"activity0";	// Надо взять из json - "activity"
			unsigned int limit = 100;
			unsigned int offset = 0;
			pRead( db, act, limit, offset, buffer );
			sprintf( sd->sp_response_buf, "{ \"errcode\": 0, \"error\": \"\", \"buffer\": [ %ls ] }", buffer.c_str() );
			sd->sp_response_buf_size = strlen(sd->sp_response_buf);
			_callback_error_code = 0;
			success = true;
			pClose( db );
		}
  }
  else if( sd->message_id == SERVER_CHAT_WRITE ) {
		// Записываем новое сообщение в базу.
		// Приходит:
		// {"sessId":"ABCDEFGH","user":"user0","projectId":"1234567890","activity":"activityCode","message":"a new message"}: 
		// Надо: 1) проверить sessId, 2) открыть базу с чатами по проекту (имя файла базы привязано к проекту),
		// 3) сделать запрос "pWrite", указав activity, user и message.

		std::wstring dbFileName{L"chat.db"}; 	// Надо прочитать из свойств проекта (проект найти по projectId)
		db = pOpen( dbFileName ); 	
		if( db != nullptr ) {	// If opened...
			std::wstring buffer; 
			std::wstring act = L"activity0";	// Надо взять из json - "activity"
			std::wstring usr = L"user0";			// Надо взять из json - "user"
			std::wstring msg = L"a message from user 0 sent through the dll"; // Надо взять из json - "message"
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
		// Обновляем сообщение, которое уже есть в базе
		// Приходит:
		// {"sessId":"ABCDEFGH","user":"user0","projectId":"1234567890","rowid":114,"message":"an updated message"}

		std::wstring dbFileName{L"chat.db"}; 	// Надо прочитать из свойств проекта (проект найти по projectId)
    db = pOpen( dbFileName ); 	
		if( db != nullptr ) {	// If opened...
			std::wstring msg = L"a message update from user 0 sent through the dll"; 	// Надо взять из json - "message"
			unsigned int rowid = 73;		// Надо взять из json - "rowid"
			status = pUpdate( db, msg, rowid );
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
		// Удаляем сообщение из чата.
		// Приходит:
		// {"sessId":"ABCDEFGH","user":"user0","projectId":"1234567890","rowid":114}

		std::wstring dbFileName{L"chat.db"}; 	// Надо прочитать из свойств проекта (проект найти по projectId)
    db = pOpen( dbFileName ); 	
		if( db != nullptr ) {	// If opened...
			unsigned int rowid = 83; 		// Надо взять из json - "rowid"
			status = pRemove( db, rowid );
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

