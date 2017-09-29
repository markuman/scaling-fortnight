# scaling-fortnight

Simple HTTP redirect-based load balancer with redis backings.


## Usage

	git clone --recursive https://github.com/markuman/scaling-fortnight
	make build

without a whimper run `./sf` 

Now setup some `endpoints` in Redis.

		127.0.0.1:6379> zadd ENDPOINT 0 http://127.0.0.1:8080
		(integer) 1
		127.0.0.1:6379> zadd ENDPOINT 0 http://127.0.0.1:8081
		(integer) 1
		127.0.0.1:6379> zadd ENDPOINT 0 http://127.0.0.1:8082
		(integer) 1
		127.0.0.1:6379> zadd ENDPOINT 0 http://127.0.0.1:8083
		(integer) 1
		127.0.0.1:6379> zadd ENDPOINT 0 http://127.0.0.1:8084
		(integer) 1

### configuration

There is one arguments. 

		sf <configfile>

The `<configfile>` is a Lua file where you can define redis connection and `scaling-fortnight` host and port.

### docker


First, run redis somehwere, somehow ... for example

		docker run -p 6379:6379 -d --name redis redis:alpine

Build docker image

		docker build --tag sf .

Start `scaling-fortnight` and link redis into it.

		docker run -p 8000:8000 --name scaling-fortnight --link redis:redis -d sf
​		

#### test

A single `pyhton2 -m SimpleHTTPServer` reached ~300 tans/sec on my i5 mobile

		siege -r 200 -c 200  http://127.0.0.1:8080/index.html
		...
		Transaction rate:	      299.52 trans/sec
		...

With `scaling-fortnight` in front of 5 `python2` Servers

		siege -r 200 -c 200  http://127.0.0.1:8000/index.html
		...
		Transaction rate:	     1556.99 trans/sec
		...

​		

# FAQ

### scaling-fortnight?

Github named it ... _(Great repository names are short and memorable. Need inspiration? How about scaling-fortnight. )_ 

### HTTP redirect?

To avoid additional network hops `scaling-fortnight` simple makes use of the HTTP redirect directive.   
With the help of the redirect directive, the server reroutes a client to another location. Instead of returning the requested object,   
the server returns the redirect response `303 See Other`. The client recognizes the new location and reissues the request. 
[[1]](http://www.javaworld.com/article/2077922/architecture-scalability/server-load-balancing-architectures-part-2-application-level-load-balanci.html)

### Redis?

Endpoints are stored in a sorted list named `ENDPOINT`.  
`scaling-fortnight` takes in every request the element with the lowest value, increase this value by 1 and use the element as redirect location.  
So you can up/down/rescale live without restart `scaling-fortnight`.

But take care:  
When you initialize a number of endpoints, you can add all with value 0.  
But when you're re-scaling and appending new endpoints, you have to  

* use the value of the lowest existing element (endpoint) `ZRANGE ENDPOINT 0 0 withscores`
* or reset all element values to zero  

Otherwise the new elements will be used until they reached the score of the lowest existing endpoint. 

# Todo

* password and databasenumber for redis connection
* add simple api/cli to re-scale endpoints
* magic-detection when an endpoint is gone? how?
* improve docker branch
* check if redisContext is still valid
* scriptable background/watch process in Lua




# cors

> If the response has an HTTP status code of 301, 302, 303, 307, or 308 Apply the cache and network error steps.

Currently it is not supported by browsers