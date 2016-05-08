FROM alpine:latest
RUN apk add --update git hiredis hiredis-dev perl musl-dev gcc\
        && cd /tmp \
        && git clone --recursive https://github.com/markuman/scaling-fortnight \
        && cd scaling-fortnight \
        && gcc -Idyad/src/ -I/usr/include/hiredis/ -lhiredis dyad/src/dyad.c -O2 main.c -o /bin/sf

# maybe cleaning up get better ...
RUN apk del gcc git perl hiredis-dev musl-dev \
        && rm -rf /var/cache/apk/* \
        && rm -rf /tmp/scaling-fortnight



ENTRYPOINT ["./bin/sf"]
CMD ["8000", "0.0.0.0"]
