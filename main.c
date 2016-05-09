// default C includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// dyad
#include "dyad.h"

// lua
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

// redis
#include <hiredis.h>
//#define DEBUG


// balancer stuff
const char* host = "127.0.0.1";
int port = 8000;
char* configfile = "config.lua";

// redis stuff
redisContext *c;
redisReply *reply;
char* redisreturn;
const char *redishostname = "127.0.0.1";
int redisport = 6379;
struct timeval timeout = { 1, 500000 }; // 1.5 seconds

static void onLine(dyad_Event *e) {
	char path[512];

	if (sscanf(e->data, "GET %127s", path) == 1) {
	
		// this has actually no impact!!
		// how to fast check if redisContext is still valid?
		// when redis is gone, scaling-fortnight will segfault with the next request...
		// when the redis connection failed -> error 503
		//if (c == NULL || c->err) {
			//dyad_writef(e->stream, "HTTP/1.1 503 Service Unavailable\r\n\r\n");
		//} else {
			
		reply = (redisReply*)redisCommand(c, "ZRANGE ENDPOINT 0 0");
		// when redis doesn't replyed with an array -> error 503
		if (reply->type == REDIS_REPLY_ARRAY && reply->elements >= 1) {
			// since we only want to get the lowest element, we're only interested in the first element
			redisreturn = reply->element[0]->str;
			// now we're increasing the lowest element by 1
			reply = (redisReply*)redisCommand(c, "ZINCRBY ENDPOINT 1 %s", redisreturn);
			// use http code 303 for redirect
			dyad_writef(e->stream, "HTTP/1.1 303 See Other\r\nLocation: %s%s\r\n\r\n", redisreturn, path);
			dyad_writef(e->stream, redisreturn );
		} else {
			printf("503");
			dyad_writef(e->stream, "HTTP/1.1 503 Service Unavailable\r\n\r\n");
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

int luaConfig() {
	// using lua for config file
	lua_State* L = luaL_newstate();
	luaL_openlibs(L);

	if (luaL_loadfile (L, configfile)) {
		return 1;
	}

	lua_getglobal(L, "redishost");
	lua_getglobal(L, "redisport");
	lua_getglobal(L, "sfhost");
	lua_getglobal(L, "sfport");
  
	redishostname = lua_tostring(L, -4);
	redisport = lua_tointeger(L, -3);
	host = lua_tostring(L, -2);
	port = lua_tointeger(L, -1);
	printf("Using Redis Host: %s\n", redishostname);
	printf("Using Redis Port: %d\n", redisport);

	lua_close(L);
	
	return 0;
}


int main(int argc, char *argv[] ) {
	
	if (argc >= 2){
		// change default host by using the 2nd argument
		configfile = argv[2];
	}
	
	printf("%s\n", configfile);
	
	// get config.lua
	if (luaConfig() == 1) {
		printf("Failed to load config.lua\n");
	}


	c = redisConnectWithTimeout(redishostname, redisport, timeout);
	if (c == NULL || c->err) {
		printf ("Cannot connect to redis\n");
		exit(1);
	}

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
