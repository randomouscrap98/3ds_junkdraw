#ifndef __HEADER_WEBSERVER__
#define __HEADER_WEBSERVER__

#include <3ds.h>
#include <stdio.h>

// Networking
#define SOC_ALIGN 0x1000
#define SOC_BUFFERSIZE 0x100000
#define SOC_MAXCLIENTS 1

#define RECV_BUF_MAX (1 << 20) // Around 1mb

#define HTTPRETURN(x, s) "HTTP/1.1 " x " " s "\r\n"
#define HTTPOK() HTTPRETURN("200", "OK")
#define HTTPNOTFOUND() HTTPRETURN("404", "NOT FOUND")

typedef struct {
  bool socInitialized;
  s32 ssock; 
  s32 csock;
	struct sockaddr_in client;
	struct sockaddr_in server;
  u8 * recv_buf;
  size_t recv_length;
} WebServer;

//char * webserver_get_extension(char * fname);
//char * webserver_get_filename(char * fname);

void webserver_init(WebServer * ws);
const char * webserver_address(WebServer * ws); 
const char * webserver_begin(WebServer * ws);
void webserver_end(WebServer * ws);
const char * webserver_recv_client(WebServer * ws, bool * received);
const char * webserver_send_client(WebServer * ws, const char * data, size_t len);
const char * webserver_send_client_file(WebServer * ws, const char * fpath, FILE * f);
void webserver_close_client(WebServer * ws);
//const char * webserver_client_get_method(WebServer * ws, char * out, char * path, size_t max_path);
const char * webserver_client_parse_request(WebServer * ws, char * method, char * path, size_t max_path);
void webserver_client_get_body_ref(WebServer * ws, u8 ** out, size_t * outlen);
const char * webserver_client_get_header(WebServer * ws, const char * header, char * out, size_t max_out);

#endif
