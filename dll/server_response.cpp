#include "helpers.h"
#include "server.h"

static constexpr int _uri_buf_size = 400;		// Buffer size for uri
static constexpr int _get_buf_size = 4000;		// Buffer size for get request
static constexpr int _html_file_path_buf_size = SRV_MAX_HTML_ROOT_PATH + 1 + _uri_buf_size + 1;

static char _image_file_name_buf[_html_file_path_buf_size+1];

static constexpr int _content_to_serve_buf_size = 10000;
static char _content_to_serve_buf[ _content_to_serve_buf_size+1];

//static constexpr int _max_response_size = 99999999;
//static constexpr int _max_response_size_digits = 8;

static char _mime_buf[MIME_BUF_SIZE + 1];

//static constexpr char _http_header_template[] = "HTTP/1.1 200 OK\r\nVersion: HTTP/1.1\r\nContent-Type:%s\r\nContent-Length:%lu\r\n\r\n";
//static constexpr int _http_header_buf_size = sizeof(_http_header_template) + MIME_BUF_SIZE + _max_response_size_digits; 

static constexpr char _http_redirect_template[] = "HTTP/1.1 302 Found\r\nLocation: %s\r\n\r\n";

static constexpr char _http_empty_message[] = "HTTP/1.1 200 OK\r\nContent-Length:0\r\n\r\n";
static constexpr char _http_header_bad_request[] = "HTTP/1.1 400 Bad Request\r\nVersion: HTTP/1.1\r\nContent-Length:0\r\n\r\n";
static constexpr char _http_header_failed_to_serve[] = "HTTP/1.1 501 Internal Server Error\r\nVersion: HTTP/1.1\r\nContent-Length:0\r\n\r\n";

static constexpr char _http_ok_options_header[] = 
		"HTTP/1.1 200 Ok\r\n"
		"Access-Control-Allow-Origin: *\r\n"
		"Access-Control-Allow-Methods: POST, GET, HEAD, OPTIONS\r\n"
		"Access-Control-Allow-Credentials: true\r\n"
		"Access-Control-Allow-Headers: cache-control, x-requested-with, Content-Type, Cookie\r\n"
		"Content-Type: text/html; charset=utf-8\r\n\r\n";


static void querySPAndPrepareResponse( callback_ptr callback, char *uri, 
	bool is_get, char *get, char *post, ResponseWrapper &response, ServerDataWrapper &sdw, char *html_root_path );
static void send_redirect( int client_socket, char *uri );


void server_response( int client_socket, char *socket_request_buf, int socket_request_buf_size, 
	char *html_source_dir, callback_ptr callback ) 
{
	static char uri[_uri_buf_size+1];
	char *post;
	bool is_get = false;
	static char get_encoded[_get_buf_size+1];
	static char get[_get_buf_size+1];
	bool is_options = false;

	int uri_status = get_uri_to_serve(socket_request_buf, uri, _uri_buf_size, &is_get, get_encoded, _get_buf_size, &post, &is_options);
	if (uri_status != 0) { 	// Failed to parse uri - closing socket...
		send(client_socket, _http_empty_message, strlen(_http_empty_message), 0);
		return;
	}
	if( is_get ) {
		int decode_status = decode_uri( get_encoded, get, _get_buf_size );
		if( decode_status != 0 ) {
			send(client_socket, _http_empty_message, strlen(_http_empty_message), 0);
			return;
		}
	}

	if( is_options ) { // An OPTIONS request - allowing all
		send(client_socket, _http_ok_options_header, strlen(_http_ok_options_header), 0);
		error_message( "sending options message:\n", _http_ok_options_header );
		return;
	}	

	ResponseWrapper response;
	ServerDataWrapper sdw;
	
	try {
		querySPAndPrepareResponse( callback, uri, is_get, get, post, response, sdw, html_source_dir );
	}
	catch (...) {
		error_message( "Failed to create response..." );
		send(client_socket, _http_header_failed_to_serve, strlen(_http_header_failed_to_serve), 0);
		return;
	}

	int send_header_result = send(client_socket, response.header, strlen(response.header), 0);
	if (send_header_result == SOCKET_ERROR) { 	// If error...
		error_message( "header send failed: ", WSAGetLastError() );
	} else {
		if (response.body != nullptr && response.body_len > 0 ) {
			int send_body_result = send(client_socket, response.body, response.body_len, 0);
			if (send_body_result == SOCKET_ERROR) { 	// If error...
				error_message( "send failed: ",  WSAGetLastError() );
			}
		} else if (response.body_allocated != nullptr && response.body_len > 0 ) {
			int send_body_result = send(client_socket, response.body_allocated, response.body_len, 0);
			if (send_body_result == SOCKET_ERROR) { 	// If error...
				error_message( "send failed: ", WSAGetLastError() );
			}
		}
	}
}


static void querySPAndPrepareResponse( callback_ptr callback, char *uri, bool is_get, char *get, char *post,
	ResponseWrapper &response, ServerDataWrapper &sdw, char *html_root_path )
{
	// Must verify if SP should respond with not a file but with a data
	int callback_return; 	// 
	bool binary_data_requested = false;

	if( strcmp( uri, "/.chat_read" ) == 0 ) {
		sdw.sd.message_id = SERVER_CHAT_READ;
		sdw.sd.message = post;
		callback_return = callback( &sdw.sd );
	} 
	else if( strcmp( uri, "/.chat_write" ) == 0 ) {
		sdw.sd.message_id = SERVER_CHAT_WRITE;			
		sdw.sd.message = post;
		callback_return = callback( &sdw.sd );
	} 
	else if( strcmp( uri, "/.chat_update" ) == 0 ) {
		sdw.sd.message_id = SERVER_CHAT_UPDATE;			
		sdw.sd.message = post;
		callback_return = callback( &sdw.sd );
	} 
	else if( strcmp( uri, "/.chat_remove" ) == 0 ) {
		sdw.sd.message_id = SERVER_CHAT_REMOVE;			
		sdw.sd.message = post;
		callback_return = callback( &sdw.sd );
	} 

	if( sdw.sd.sp_response_buf == nullptr || 	// Might happen if mistakenly left as nullptr in SP.
		callback_return < 0 || sdw.sd.sp_response_buf_size == 0 || // An error 
		sdw.sd.sp_response_buf_size > _max_response_size ) 	 // The response is too big
	{
		strcpy(response.header, _http_header_bad_request); 
		error_message( "Bad Request..." );
	} else {
		if( binary_data_requested ) { 	// An image? (or other binary data for future use)
			set_mime_type(uri, _mime_buf, MIME_BUF_SIZE);
		} else { 	// 
			strcpy_s( _mime_buf, MIME_BUF_SIZE, "text/json; charset=utf-8" );
		}
		sprintf_s( response.header, _http_header_buf_size, 
			_http_header_template, _mime_buf, (unsigned long)sdw.sd.sp_response_buf_size );
		response.body = sdw.sd.sp_response_buf;
		response.body_len = sdw.sd.sp_response_buf_size;
		error_message( "header:\n", response.header,  "body:\n", response.body );
	}
	return;
}


static void send_redirect( int client_socket, char *uri ) {	
	int required_size = sizeof(_http_redirect_template) + strlen(uri);
	if(  required_size >= _content_to_serve_buf_size ) {
		send( client_socket, _http_header_bad_request, sizeof(_http_header_bad_request), 0 );
	} else {
		sprintf_s( _content_to_serve_buf, _content_to_serve_buf_size, _http_redirect_template, uri);		
		send( client_socket, _content_to_serve_buf, strlen(_content_to_serve_buf), 0 );
 	}
}

