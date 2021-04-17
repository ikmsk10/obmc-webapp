# Please build bmcweb-base first with
# docker build --no-cache --force-rm -t obmc-webapp-base -f Dockerfile.base .
FROM obmc-webapp-base

RUN mkdir -p /source/build
RUN mkdir -p /usr/share/www
RUN mkdir -p /etc/lighttpd.d


CMD ["/bin/bash", "-c", "cd /source/build && meson .. && ninja && ninja test && lighttpd -D -f /source/lighttpd.conf"]
#CMD ["/bin/bash"]
