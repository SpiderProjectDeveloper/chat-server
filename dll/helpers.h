#pragma once

void set_mime_type(char *fn, char *mime_buf, int mime_buf_size);

int get_uri_to_serve(char *b, char *uri_buf, int uri_buf_size, bool *is_get, char *get_buf, int get_buf_size, char **post, bool *is_options);

bool is_html_request( char *uri );

int decode_uri( char *uri_encoded, char *uri, int buf_size );

int get_content_read( char *b, int b_len );

int get_content_length( char *b, int b_len );


template<class... Args>
void error_message( Args... args ) {
  return;
	///*
  //#ifndef __DEV__
	//	return;
	//#endif
	//(std::cout << ... << args) << std::endl;
	std::fstream log_file("c:\\Users\\1395262\\Desktop\\sava\\spider\\log.txt", std::fstream::out | std::fstream::app);
	if( log_file ) {
			(log_file << ... << args) << std::endl;
		log_file.close();
	}
	//*/
}
