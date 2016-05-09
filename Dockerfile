FROM ubuntu:16.04

RUN apt update -y && \
        apt install git libhiredis0.13 libhiredis-dev tcc -y \
        && cd /tmp \
        && git clone --recursive https://github.com/markuman/scaling-fortnight \
        && cd scaling-fortnight \
        && tcc -Idyad/src/ -I/usr/include/hiredis/ -lhiredis dyad/src/dyad.c main.c -o /bin/sf \
        && mkdir /config && cp config.lua /config/

RUN apt remove git tcc libhiredis-dev -y \
        && apt-get autoremove -y \
        && apt-get clean


ENTRYPOINT ["./bin/sf"]
CMD ["/config/config.lua"]
