# tetra-test
just some test code for the tetra concept

Basic usage: 

Build the tetra library and two C test servers: 

./build_all.sh


Run the simple accept()-based server (listens on port 8000): 

LD_PRELOAD=./libtetra.so ./simple-server

Run the select()-based server (listens on port 8000): 

LD_PRELOAD=./libtetra.so ./select-server

Building and Running the Docker Container: 

docker build -t basic-python-web .

docker run -it -p 8000:8000 -v /Users/danwent/tetra:/tetra basic-python-web

