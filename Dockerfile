FROM python:3-onbuild
RUN apt-get update && apt-get -y install build-essential python3-dev python3-setuptools python3-numpy python3-scipy python3-pip libatlas-dev libatlas3gf-base
RUN pip3 install numpy
RUN pip3 install scipy
RUN pip3 install scikit-learn
RUN update-alternatives --set libblas.so.3 /usr/lib/atlas-base/atlas/libblas.so.3
RUN update-alternatives --set liblapack.so.3 /usr/lib/atlas-base/atlas/liblapack.so.3
WORKDIR /tetra
CMD /bin/bash 
