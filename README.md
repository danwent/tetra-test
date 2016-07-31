# tetra-test
just some test code for the tetra concept

Basic usage: 

Build the tetra library and two C test servers: 

./build_all.sh


Run the simple accept()-based server (listens on port 8000): 

LD_PRELOAD=./libtetra.so ./simple-server

Run the select()-based server (listens on port 8000): 

LD_PRELOAD=./libtetra.so ./select-server

In another window, hit localhost 8000 with HTTP request with a query
string that has integer values for parameters 'a', 'b', and 'c'.  Ex: 

wget -qO- 'http://localhost:8000/foo/abc?a=10&b=3&c=4'

The latency of such a call will be logged to tetra-data.txt in the local
directory that the webserver is running in.  

Building and Running the Docker Container: 

docker build -t basic-python-web .

docker run -it -p 8000:8000 -v /Users/danwent/tetra:/tetra basic-python-web

