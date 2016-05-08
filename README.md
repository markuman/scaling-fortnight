# scaling-fortnight

Simple HTTP redirect-based load balancer with redis backings.


## Usage

	git clone --recursivly https://github.com/markuman/scaling-fortnight
	make
	
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

There are two arguments. 

		sf <port> <host>
		
Currently you cannot change the redis connection details. It will connect to `127.0.0.1:6379`

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
`scaling-fortnight` takes with every request the element with the lowest value, increase this value by 1 and use the element as redirect location.  
So you can up/down/rescale live without restart `scaling-fortnight`.

But take care:  
When you initialize a number of endpoints, you can add all with value 0.  
But when you re-scaling and append new endpoints, you have to  

* use the value of the lowest existing element (endpoint) `ZRANGE ENDPOINT 0 0 withscores`
* or reset all element values to zero  
    
Otherwise the new elements will be used until they reached the score of the lowest existing endpoint. 

# Todo

* set individual redis connection
* add simple api/cli to re-scale endpoints
* magic-detection when an endpoint is gone? how?

    
