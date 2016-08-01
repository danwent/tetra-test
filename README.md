# tetra-test
just some test code for the tetra concept

Basic usage: 


Building and Running the dev Docker Container: 

docker build -t tetra-dev .

docker run -it -p 8000:8000 -v ~/tetra:/tetra tetra-dev

Build the tetra library and two C test servers: 

./build_all.sh

Run the simple accept()-based server (listens on port 8000): 

LD_PRELOAD=./libtetra.so ./simple-server

Run the select()-based server (listens on port 8000): 

LD_PRELOAD=./libtetra.so ./select-server

In another window, hit localhost 8000 with multiple HTTP requests 
with a query string that has integer values for parameters 
'a', 'b', and 'c'.  Ex: 

wget -qO- 'http://localhost:8000/foo/abc?a=10&b=3&c=4'

The latency of such a call will be logged to tetra-data.txt in the local
directory that the webserver is running in. 

Then run the following command to do a linear regression on the values
in tetra-data.txt :   

python tetra_linear_regression.py 

With simple-server, it should show that the latency 
is O(n^2) for a, O(n) for b, and O(1) for c.  select-server is just O(1) 
regardless of params.   

Example output is like: 

raw_X = [(10, 10, 10), (50, 10, 50), (0, 10, 50), (0, 10, 100), (0, 200, 100), (5, 200, 100), (5, 0, 100), (0, 0, 100), (0, 0, 500)]
y = [25910, 696126, 10210, 3442, 46646, 63473, 6065, 215, 262]
1   658.5
a   -1.66
a**2    223.67
a**3    1.05
b   451.92
b**2    -0.92
b**3    -0.0
c   13.61
c**2    -0.17
c**3    0.0
