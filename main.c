// default C includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// dyad
#include "dyad.h"

// redis
#include <hiredis.h>
//#define DEBUG


// balancer stuff
const char* host = "127.0.0.1";
int port = 8000;

// redis stuff
redisContext *c;
redisReply *reply;
char* redisreturn;
const char *redishostname = "127.0.0.1";
int redisport = 6379;
struct timeval timeout = { 1, 500000 }; // 1.5 seconds

static void onLine(dyad_Event *e) {
	char path[2000];

	if (sscanf(e->data, "GET %127s", path) == 1) {
	
		// when the redis connection failed -> error 503
		if (c == NULL || c->err) {
			dyad_writef(e->stream, "HTTP/1.1 503 Service Unavailable\r\n\r\n");
			if (c) {
				redisFree(c);
				dyad_end(e->stream);
			} else {
				dyad_end(e->stream);
			}
		} else {
			reply = (redisReply*)redisCommand(c, "ZRANGE ENDPOINT 0 0");
			// when redis doesn't replyed with an array -> error 503
			if (reply->type == REDIS_REPLY_ARRAY) {
				// since we only want to get the lowest element, we're only interested in the first element
				redisreturn = reply->element[0]->str;
				// now we're increasing the lowest element by 1
				reply = (redisReply*)redisCommand(c, "ZINCRBY ENDPOINT 1 %s", redisreturn);
				// use http code 303 for redirect
				dyad_writef(e->stream, "HTTP/1.1 303 See Other\r\nLocation: %s%s\r\n\r\n", redisreturn, path);
				dyad_writef(e->stream, redisreturn );
			} else {
				dyad_writef(e->stream, "HTTP/1.1 503 Service Unavailable\r\n\r\n");
			}
	
		}
	}
	
	dyad_end(e->stream);	
}


static void onAccept(dyad_Event *e) {
  dyad_addListener(e->remote, DYAD_EVENT_LINE, onLine, NULL);
}

static void onListen(dyad_Event *e) {
  printf("server listening: http://%s:%d\n", host, dyad_getPort(e->stream));
}

static void onError(dyad_Event *e) {
  printf("server error: %s\n", e->msg);
}


int main(int argc, char *argv[] ) {

  // change default port 8000 by using first input argument
  if (argc >= 2){
    port = atoi(argv[1]);
  }

  // change default host by using the 2nd argument
  if (argc >= 3){
	host = argv[2];
  }

  c = redisConnectWithTimeout(redishostname, redisport, timeout);

  dyad_Stream *s;
  dyad_init();

  s = dyad_newStream();
  dyad_addListener(s, DYAD_EVENT_ERROR,  onError,  NULL);
  dyad_addListener(s, DYAD_EVENT_ACCEPT, onAccept, NULL);
  dyad_addListener(s, DYAD_EVENT_LISTEN, onListen, NULL);
  dyad_listenEx(s, host, port, 16);

  while (dyad_getStreamCount() > 0) {
    dyad_update();
  }

  return 0;
}
