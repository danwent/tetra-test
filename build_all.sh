gcc -shared -fPIC -ldl tetra.c -o libtetra.so
gcc simple-server.c -o simple-server
gcc select-server.c -o select-server
