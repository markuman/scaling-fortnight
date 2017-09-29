-- scaling-fortnight
sfhost = "0.0.0.0"
sfport = 8001

-- redis database
redishost = "127.0.0.1"
redisport = 6379

-- after how many number of requests, watch.lua should be executed?
-- set to zero to disable this feature
NumberOfRequests = 0

-- use cors headers
-- 0 = false, 1 = true
use_cors = 1
origin =  "*"
methods = "GET, POST, PUT"
headers = "Content-Type"
