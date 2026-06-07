#include "webserver.h"
#include "log.h"

#include <string.h>
#include <stdio.h>

#include <fcntl.h>
#include <malloc.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static u32 *SOC_buffer = NULL;
// static char http_200[] = HTTPRETURN("200", "OK");

static char webserve_error[1024];

#define _WEBERR(f, ...) snprintf(webserve_error, 1024, f, ##__VA_ARGS__)
//return webserve_error;

void webserve_initsocbuf() {
  if(SOC_buffer == NULL) {
    SOC_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);
  }
}

char * webserver_get_byend(char * fname, char last) {
  char * extension = fname + strlen(fname);
  while(--extension != fname) {
    if(*extension == last) {
      extension++;
      break;
    }
  }
  return extension;
}

char * webserver_get_filename(char * fname) {
  return webserver_get_byend(fname, '/');
}

char * webserver_get_extension(char * fname) {
  return webserver_get_byend(fname, '.');
}

const char * webserver_address(WebServer * ws) {
  return inet_ntoa(ws->server.sin_addr);
}

#define _SOCKBLOCK(sock, block) { \
	int socflags = fcntl(sock, F_GETFL, 0); \
  if(socflags < 0) { \
	  _WEBERR("fcntl get: %d %s\n", errno, strerror(errno)); \
    goto CLEANUP; \
  } \
  socflags = block ? (socflags & ~O_NONBLOCK) : (socflags | O_NONBLOCK); \
	if(fcntl(sock, F_SETFL, socflags) < 0) { \
	  _WEBERR("fcntl set: %d %s\n", errno, strerror(errno)); \
    goto CLEANUP; \
  } \
}

#define _SOCKCHECK(result, name) { \
  if(result < 0) { \
    _WEBERR(name": %d %s\n", errno, strerror(errno)); \
    goto CLEANUP; \
  } \
}

void webserver_init(WebServer * ws) {
  ws->recv_buf = NULL;
  ws->ssock = -1;
  ws->csock = -1;
  ws->socInitialized = false;
}

const char * webserver_begin(WebServer * ws) {

  if(ws->ssock >= 0 || ws->csock >= 0 || ws->socInitialized) {
    _WEBERR("Webserver already running");
    goto CLEANUP;
  }

  ws->recv_buf = malloc(sizeof(char) * RECV_BUF_MAX);

  if(!ws->recv_buf) {
    _WEBERR("Can't allocate recv buffer");
    goto CLEANUP;
  }

  webserve_initsocbuf();
  if(SOC_buffer == NULL) {
    _WEBERR("Can't allocate SOC buf");
    goto CLEANUP;
  }
  int ret;
  if((ret = socInit(SOC_buffer, SOC_BUFFERSIZE)) != 0) {
    _WEBERR("Can't init SOC");
    goto CLEANUP;
  }
  ws->socInitialized = true;

  // ws->client_len = sizeof(ws->client);
	ws->ssock = socket (AF_INET, SOCK_STREAM, IPPROTO_IP);
	if (ws->ssock < 0) {
    _WEBERR("socket: %d %s", errno, strerror(errno));
    goto CLEANUP;
	}

	memset (&ws->server, 0, sizeof (ws->server));
	memset (&ws->client, 0, sizeof (ws->client));

	ws->server.sin_family = AF_INET;
	ws->server.sin_port = htons (80);
	ws->server.sin_addr.s_addr = gethostid();

  if ( (ret = bind (ws->ssock, (struct sockaddr *) &ws->server, sizeof (ws->server))) ) {
    _WEBERR("bind: %d %s\n", errno, strerror(errno));
    goto CLEANUP;
	}

  // Set socket non blocking so we can still read input to exit
  _SOCKBLOCK(ws->ssock, 0);

  // Begin listen, let N clients connect at once
	if ( (ret = listen( ws->ssock, SOC_MAXCLIENTS)) ) {
		_WEBERR("listen: %d %s\n", errno, strerror(errno));
    goto CLEANUP;
	}

  return NULL;

CLEANUP:
  return webserve_error;
}

void webserver_end(WebServer * ws) {
  if(ws->ssock>0) { 
    LOGTRACE("Closing socket %d", ws->ssock);
    close(ws->ssock); 
    ws->ssock = -1;
  }
  if(ws->csock>0) { 
    LOGTRACE("Closing csocket %d", ws->csock);
    close(ws->csock); 
    ws->csock = -1;
  }
  if(ws->socInitialized) {
    LOGTRACE("Shutting down SOC");
    socExit();
    ws->socInitialized = false;
  }
  if(ws->recv_buf) {
    free(ws->recv_buf);
  }
}

const char * webserver_recv_client(WebServer * ws, bool * received) {
  *received = false;
  u32 clientlen = sizeof(ws->client);
  ws->csock = accept (ws->ssock, (struct sockaddr *) &ws->client, &clientlen);

  if (ws->csock<0) {
    if(errno != EAGAIN) {
      _WEBERR("accept: %d %s\n", errno, strerror(errno));
      goto CLEANUP;
    }
  } else {
    // set client socket to blocking to simplify sending data back
    _SOCKBLOCK(ws->csock, 1);
    LOGDBG("Connecting port %d from %s\n", ws->client.sin_port, inet_ntoa(ws->client.sin_addr));
    // This might be slow?
    memset (ws->recv_buf, 0, RECV_BUF_MAX);

    int ret = recv (ws->csock, ws->recv_buf, RECV_BUF_MAX - 2, 0);
    LOGDBG("RECV: %d bytes", ret);
    *received = true;
  }

  return NULL;
CLEANUP:
  return webserve_error;
}

const char * webserver_send_client(WebServer * ws, const char * data, size_t len) {
  _SOCKCHECK(send(ws->csock, data, len, 0), "send");
  return NULL;
CLEANUP:
  return webserve_error;
}

void webserver_close_client(WebServer * ws) {
  close(ws->csock);
  LOGDBG("Closed %s", inet_ntoa(ws->client.sin_addr));
  ws->csock = -1;
}
