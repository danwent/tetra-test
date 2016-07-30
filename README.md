# tetra-test
just some test code for the tetra concept

Basic usage: 

Build the tetra library and two C test servers: 

./build_all.sh


Run the simple accept()-based server (listens on port 8000): 

LD_PRELOAD=./libtetra.so ./simple-server

Run the select()-based server (listens on port 8000): 

LD_PRELOAD=./libtetra.so ./select-server


