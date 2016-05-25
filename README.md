# Cache-server
A basic proxy cache server

The code is written in C.

Compile using the make command on bash.
Run program by: - ./p2 <port_number>
Eg- ./p2 8080

It then operates on port 8080 as a proxy server, serving all the files present in it's cache file - "cache.txt".
If the file is not found on this server, it downloads and caches it from the remote server, and makes an entry in cache.txt.
All the requests to the remote server are made in parallel through pthreads which are equal in number to (size of file in bytes)/500. This can be improved upon
The server is made concurrent by spawning a child process for every inbound request.

Don't forget to change proxy settings in your browser to use it!

Some issues to be handled -
	1) File Download from the proxy cache server doesn't work in some browsers
	2) Need to add facility to view html pages, even they are just ignored in the version.
