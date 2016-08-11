# tetra-test
just some test code for the tetra concept

# Setup

Building and Running the dev Docker Container: 

git clone https://github.com/danwent/tetra-test 

cd tetra-test

docker build -t tetra-dev .

Note: if you didn't clone in your home directory, you will need to update
the volume portion of the docker run command below to point to your tetra-test
directory

docker run -it -p 8000:8000 -v ~/tetra-test:/tetra tetra-dev

Note: the following steps happen inside the container.

Build the tetra library and two C test servers: 

./build_all.sh

# Running with linear regression 

Run the simple accept()-based server (listens on port 8000): 

LD_PRELOAD=./libtetra.so ./simple-server

In another window, hit localhost 8000 with multiple HTTP requests 
with a query string that has different integer values for parameters 
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

```
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
```

# Other server examples

## C  (select) 

Run the select()-based server written in C (listens on port 8000): 

LD_PRELOAD=./libtetra.so ./select-server

## Python (basic) 

Python3 should already be installed in the Docker image.  Just run: 

LD_PRELOAD=./libtetra.so python -m http.server 

## NodeJS (basic) 

Note: this doesn't work.  I finally figured out that this is because 
nodejs does not use libc to call things like accept(), and instead issues
a sys-call directly using the sys-call function.  We shoud be able to 
hook the syscall function and interpose there as well.  With nodejs, this is
implemented using libuv:  https://github.com/libuv/libuv/blob/v1.x/src/unix/linux-syscalls.c#L241


apt-get install -y nodejs npm 

git clone https://github.com/heroku/node-js-getting-started.git

cd node-js-getting-started

npm install

LD_PRELOAD=../libtetra.so PORT=8000 nodejs index.js 

## Ruby-on-Rails 

cd

git clone https://github.com/rbenv/rbenv.git ~/.rbenv

echo 'export PATH="$HOME/.rbenv/bin:$PATH"' >> ~/.bashrc

echo 'eval "$(rbenv init -)"' >> ~/.bashrc

exec $SHELL

git clone https://github.com/rbenv/ruby-build.git ~/.rbenv/plugins/ruby-build

echo 'export PATH="$HOME/.rbenv/plugins/ruby-build/bin:$PATH"' >> ~/.bashrc

exec $SHELL

rbenv install 2.3.1

rbenv global 2.3.1

gem install bundler

rbenv rehash

gem install rails -v 4.2.6

rails new my-rails-app

cd my-rails-app

bundle install

rails generate controller welcome

[edit app/views/welcome/index.html.erb to include "hello world"]

[ in file config/routes.rb, uncomment the following line:   root 'welcome#index' ] 

LD_PRELOAD=../libtetra.so rails server -p8000

Note: it is recommended to access this server via localhost:8000 due to some
rails wierdness that I have yet to investigate. 

## Java Spring Boot


Note:  This isn't working right now, we never see the 'Hello from Groovy app!'
printed, nor do we see the latency info of the child thread closing out.  
After a little stracing, it seems like this is because spring actually is doing a lot
of forking, and we know the simple PoC code isn't going to handle that well.  The 
webserver itself seems to be running in a forked process, and then it seems that it may
be the cause that each web request already results in another short-lived forked process. 
We'll have to look at this in more detail.  

apt-get install default-jdk maven

wget http://repo.spring.io/release/org/springframework/boot/spring-boot-cli/1.1.4.RELEASE/spring-boot-cli-1.1.4.RELEASE-bin.tar.gz

tar -xzf spring-boot-cli-1.1.4.RELEASE-bin.tar.gz 

cd spring-1.1.4.RELEASE

Create a file called app.groovy with the following contents: 

```
@RestController
public class BasicController {

    @RequestMapping("/")
    String home() {
        System.out.println("Hello from Groovy app!");
        "Hello World!"
    }

}
```

Create a file called application.properties with the following contents: 

```
server.port=8000
```

LD_PRELOAD=../libtetra.so ./bin/spring run app.groovy

## TODO: 
- Get data points on actual COW memory overhead.  I think we can use 
/proc/<pid>/smaps, which will distinguish between 'shared' pages and
'dirty' pages.  Dirty is what we care about, as they are unique to the 
child process. 
- Test with Java
- Test with Golang
- use peek to capture and serialize actual JSON body, not just URL params. 
- figure out how peek logic will work with SSL encrpytion.  Presumably we
need to override openssl read calls to get the plain-text? 
- Capture total memory + CPU used. 
- Capture other "inputs" that would impact our modeling for latency, including
database latency + inputs (parse db client protocol using peek), and remote
webservice calls latency + inputs (to start, just assume JSON). 


