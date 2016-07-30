FROM python:3-onbuild
WORKDIR /tetra
#CMD gcc -shared -fPIC -ldl tetra.c -o libtetra.so && gcc simple-server.c -o simple-server && export LD_PRELOAD=./libtetra.so && ./simple-server
CMD bin/bash 
