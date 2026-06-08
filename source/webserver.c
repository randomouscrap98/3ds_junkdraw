#include "webserver.h"
#include "log.h"

#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include <fcntl.h>
#include <malloc.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static u32 *SOC_buffer = NULL;

static char webserve_error[1024];

#define _WEBERR(f, ...) snprintf(webserve_error, 1024, f, ##__VA_ARGS__)
//return webserve_error;

void webserve_initsocbuf() {
  if(SOC_buffer == NULL) {
    SOC_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);
  }
}

const char * webserver_get_byend(const char * fname, char last) {
  const char * extension = fname + strlen(fname);
  while(--extension != fname) {
    if(*extension == last) {
      extension++;
      break;
    }
  }
  return extension;
}

const char * webserver_get_filename(const char * fname) {
  return webserver_get_byend(fname, '/');
}

const char * webserver_get_extension(const char * fname) {
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

  ws->recv_buf = malloc(sizeof(u8) * RECV_BUF_MAX);
  ws->recv_length = 0;

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

    ws->recv_length = 0;
    ssize_t this_read = 0;
    size_t contentLength = 0;
    u8 * bodyStart = NULL;

    while(true) {
      this_read = 
           recv(ws->csock, ws->recv_buf + ws->recv_length, RECV_BUF_MAX - 2 - ws->recv_length, 0);
      if(this_read <= 0) {
        break;
      }
      ws->recv_length += this_read;
      ws->recv_buf[ws->recv_length] = 0;
      if(bodyStart == NULL) { // Haven't found the body
        // This is slow but whatever
        size_t bodylen;
        webserver_client_get_body_ref(ws, &bodyStart, &bodylen);
        if(bodyStart) {
          // We have a body now, let's see if we have a content length. If so, keep reading, otherwse quit
          char headerval[128];
          if(webserver_client_get_header(ws, "Content-Length", headerval, 128)) {
            goto CLEANUP;
          }
          if(strlen(headerval) == 0) {
            break; // No content length? fine...?
          } else {
            contentLength = atol(headerval);
            //LOGDBG("FOUND: %zu (%s)", contentLength, headerval);
          }
        } 
      } else {
        if((ws->recv_buf + ws->recv_length) - bodyStart >= contentLength) {
          break;
        }
      }
    }

    if(this_read < 0) {
      _WEBERR("recv: %d %s\n", errno, strerror(errno));
      goto CLEANUP;
    }

    LOGDBG("RECV: %d bytes", ws->recv_length);
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

const char * webserver_send_client_file(WebServer * ws, const char * fpath, FILE * f) {
  char temp[4096];
  char contenttype[16] = "image";
  const char * extension = webserver_get_extension(fpath);
  const char * filename = webserver_get_filename(fpath);
  if(strcmp(extension, "html") == 0) {
    strcpy(contenttype,"text");
  }
  sprintf(temp, "%sContent-type: %s/%s\r\nContent-Disposition: inline; "
          "filename=\"%s\"\r\n\r\n", HTTPOK(), contenttype, extension, filename);
  if(webserver_send_client(ws, temp, strlen(temp))) {
    goto CLEANUP;
  }

  // Now read from the given file into a small buffer and send it over and over.
  if(fseek(f, 0, SEEK_SET)) {
    _WEBERR("Failed to seek to start: %s", fpath);
    goto CLEANUP;
  }
  while (1) {
    unsigned long count = fread(temp, 1, sizeof(temp) - 2, f);
    if(count) {
      if(webserver_send_client(ws, temp, count)) {
        goto CLEANUP;
      }
    }
    if(count != sizeof(temp) - 2) {
      if (!feof(f)) {
        _WEBERR("Failed to read file %s", fpath);
        goto CLEANUP;
      }
      break;
    } 
  }
  return NULL;
CLEANUP:
  return webserve_error;
}

const char * webserver_client_parse_request(WebServer * ws, char * method, char * path, size_t max_path) {
  method[0] = 0;
  int i = 0;
  while(i < ws->recv_length && ws->recv_buf[i] != ' ') {
    method[i] = toupper(ws->recv_buf[i]);
    method[++i] = 0;
    if(i > 8) {
      _WEBERR("Ill-formed request!");
      return webserve_error;
    }
  }
  // Now that we have the method, copy the entire path out (skipping spaces)
  while(i < ws->recv_length && ws->recv_buf[i] == ' ') {
    i++;
  }
  int si = i;
  while(i < ws->recv_length && ws->recv_buf[i] != ' ' && (i - si) < max_path - 1) {
    path[i - si] = ws->recv_buf[i];
    path[++i - si] = 0;
  }
  if(i == si) {
    _WEBERR("Ill-formed request!");
    return webserve_error;
  }
  return NULL;
}

void webserver_client_get_body_ref(WebServer * ws, u8 ** out, size_t *outlen) {
  //char * b = (char *)(ws->recv_buf + (ws->recv_length - 4));
  //LOGDBG("LAST: %d %d %d %d", b[0], b[1], b[2], b[3]);
  char * found = strstr((char *)ws->recv_buf, "\r\n\r\n");
  //*out = strnstr((char *)ws->recv_buf, "\r\n\r\n", ws->recv_length);
  if(found) {
    *out = (u8 *)(found + 4); // skip the characters and get right to the body
    *outlen = ws->recv_length - (*out - ws->recv_buf);
  } else {
    *out = NULL;
    *outlen = 0;
  }
}

const char * webserver_client_get_header(WebServer * ws, const char * header, char * out, size_t max_out) {
  char lowhead[128];
  char nexthead[128];
  int headlen = strlen(header);
  if(!headlen || max_out == 0) {
    _WEBERR("No header passed to check?");
    return webserve_error;
  }
  lowhead[headlen] = 0;
  for(int i = 0; i < headlen; i++) {
    lowhead[i] = tolower(header[i]);
  }
  // Now start at the beginning, scan through until we get an empty header, look
  // for the lowercase variant of this header
  char * this = strstr((char *)ws->recv_buf, "\r\n");
  if(this) {
    this += 2;
  } else {
    _WEBERR("No headers beyond request line??");
    return webserve_error;
  }
  char * next;
  while((next = strnstr(this, "\r\n", RECV_BUF_MAX - (this - (char *)ws->recv_buf)))) {
    int i = 0;
    nexthead[0] = 0;
    while(this[i] != ':' && (this + i) != next && i < 127) {
      nexthead[i] = tolower(this[i]);
      nexthead[++i] = 0;
    }
    if(strlen(nexthead) == 0) {
      // This is safe; just return 0
      LOGDBG("Header not found: %s", header);
      out[0] = 0;
      return NULL;
    }
    if(strcmp(nexthead, lowhead) == 0) {
      // Copy the value in
      this += i;
      while(*this == ' ' || *this == ':') this++;
      i = 0;
      while((this + i) != next && i < max_out - 1) {
        out[i] = this[i];
        i++;
      }
      out[i] = 0;
      return NULL;
    }
    this = next + 2;
  }
  _WEBERR("Header doesn't end?");
  return webserve_error;
}

void webserver_close_client(WebServer * ws) {
  close(ws->csock);
  LOGDBG("Closed %s", inet_ntoa(ws->client.sin_addr));
  ws->csock = -1;
}
