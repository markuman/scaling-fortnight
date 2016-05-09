-- doing some background stuff? e.g. rescaling, tidy up endpoints in redis
function watch ()

	local file = io.open ("/tmp/sf.watch", "w")
	
	file:write (os.date ())
	
	file:close ()

end
