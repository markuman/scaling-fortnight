// default C includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// redis lib include
#include <hiredis/hiredis.h>
// lua includes. used for config and background processing
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
// to run lua background process
#include <pthread.h>
// asynchronous networking lib
#include "dyad/src/dyad.h"


// default connections
const char* host = "127.0.0.1";
int port = 8000;
// default redis connections
const char *redishostname = "127.0.0.1";
int redisport = 6379;
// use cors
int use_cors = 0;
const char* origin = "*";
const char* methods = "GET, POST, PUT, OPTIONS";
const char* headers = "Content-Type";

// redis stuff
redisContext *c;
redisReply *reply;
char* redisreturn;

pthread_t thread_handle;
const char* configfile = "config.lua";
unsigned long int count = 0;
unsigned long int NumberOfRequests = 0;

void *luaThread() {
	lua_State* LT = luaL_newstate();
	luaL_openlibs(LT);
	if( luaL_loadfile(LT, "watch.lua") || lua_pcall(LT, 0, 0, 0) ) {
		return NULL;
	}
	lua_getglobal(LT, "redishost");
	lua_getglobal(LT, "watch");
	lua_pcall(LT, 0, 0, 0) ;
	lua_close(LT);
	return NULL;
}

int watch() {
	if (NumberOfRequests > 0) {
		count++;
		if ((count % NumberOfRequests) == 0) {
			count = 0;
			if(pthread_create(&thread_handle, NULL, luaThread, NULL)) {
				fprintf(stderr, "Error creating thread\n");
				return 1;
			}
			pthread_detach(thread_handle);	
		}
	}
	return 0;
}


static void onLine(dyad_Event *e) {
	char path[512];
	printf("%s\n\n", e->data);

	if (use_cors == 0) {
		if (strstr(e->data, "OPTIONS") != NULL) {
			printf("reply with cors header now\n");
			dyad_writef(e->stream, "HTTP/1.1 204 No Content\r\n");
			dyad_writef(e->stream, "Access-Control-Allow-Origin: %s\r\n", origin);
			dyad_writef(e->stream, "Access-Control-Allow-Methods: %s\r\n", methods);
			dyad_writef(e->stream, "Access-Control-Allow-Headers: %s\r\n", headers);
			dyad_writef(e->stream, "Content-Length: 0\r\n");
			dyad_writef(e->stream, "Connection: keep-alive\r\n\r\n");
			dyad_end(e->stream);
			return;
		}
	}
	
	if ((sscanf(e->data, "GET %127s", path) == 1) || (sscanf(e->data, "POST %127s", path) == 1) || (sscanf(e->data, "OPTIONS %127s", path) == 1)) {
		printf("yes\n");
		watch();
		reply = (redisReply*)redisCommand(c, "ZRANGE ENDPOINT 0 0");
		// when the redis connection failed -> error 503
		if (reply != NULL) {
			// when redis doesn't replyed with an array -> error 503
			if ((reply->type == REDIS_REPLY_ARRAY) && (reply->elements > 0)) {
				// since we only want to get the lowest element, we're only interested in the first element
				redisreturn = reply->element[0]->str;
				// now we're increasing the lowest element by 1
				reply = (redisReply*)redisCommand(c, "ZINCRBY ENDPOINT 1 %s", redisreturn);
				// use http code 303 for redirect
				dyad_writef(e->stream, "HTTP/1.1 307 Temporary Redirect\r\n");
				dyad_writef(e->stream, "Access-Control-Allow-Origin: %s\r\n", origin);
				//if (use_cors == 1) {
				//	dyad_writef(e->stream, "Access-Control-Allow-Origin: %s\r\n", origin);
				//	dyad_writef(e->stream, "Access-Control-Allow-Methods: %s\r\n", methods);
				//	dyad_writef(e->stream, "Access-Control-Allow-Headers: %s\r\n", headers);
				//}
				dyad_writef(e->stream, "Location: %s%s\r\n\r\n", redisreturn, path);
				dyad_writef(e->stream, redisreturn );
				dyad_end(e->stream);
				return;	
			}
		}
	}
	dyad_writef(e->stream, "HTTP/1.1 503 Service Unavailable\r\n\r\n");
	dyad_end(e->stream);
	return;	
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

	if( luaL_loadfile(L, configfile) || lua_pcall(L, 0, 0, 0) ) {
		printf("Failed to load config %s\n", configfile);
		return 1;
	}

	lua_getglobal(L, "redishost");
	redishostname = lua_tostring(L, -1);
	lua_pop(L, lua_gettop(L));
	
	lua_getglobal(L, "redisport");
	redisport = lua_tointeger(L, -1);
	lua_pop(L, lua_gettop(L));
	
	lua_getglobal(L, "sfhost");
	host = lua_tostring(L, -1);
	lua_pop(L, lua_gettop(L));
	
	lua_getglobal(L, "sfport");
	port = lua_tointeger(L, -1);
	lua_pop(L, lua_gettop(L));
	
	lua_getglobal(L, "NumberOfRequests");
	NumberOfRequests = lua_tointeger(L, -1);
	lua_pop(L, lua_gettop(L));
	
	lua_getglobal(L, "use_cors");
	use_cors = lua_tointeger(L, -1);
	lua_pop(L, lua_gettop(L));

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

	printf("Using Redis Host: %s\n", redishostname);
	printf("Using Redis Port: %d\n", redisport);
	c = redisConnect(redishostname, redisport);
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
