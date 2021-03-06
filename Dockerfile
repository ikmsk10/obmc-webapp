# docker build --no-cache --force-rm -t obmc-webapp-base -f Dockerfile.base .
FROM obmc-webapp-base

RUN mkdir -p /source/build
RUN mkdir -p /usr/share/www
RUN mkdir -p /etc/lighttpd.d
RUN mkdir -p /home/root
RUN mkdir -p /root/.ssh/
RUN mkdir -p /etc/ssl/certs/https/

ADD .ssh/ /root/.ssh/
ADD tests/conf/bmcweb_persistent_data.json /home/root/

CMD ["/bin/bash", "-c", "cd /source/build && meson .. && ninja && ninja test && /source/configuration/ssl/init_default_x509.sh && lighttpd -D -f /source/tests/conf/lighttpd.conf"]
