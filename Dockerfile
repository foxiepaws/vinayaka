FROM ubuntu:16.04

LABEL maintainer="https://github.com/distsn/vinayaka" \
      description="語彙の類似からマストドンのユーザーを推挙するウェブアプリケーション"

EXPOSE 80

WORKDIR /vinayaka

RUN apt-get update \
 && apt-get install -y \
    git \
    make \
    gcc \
    g++ \
    libcurl4-openssl-dev \
    apache2 \
    cron \
 && rm -rf /etc/apache2/sites-enabled/000-default.conf \
 && ln -s /etc/apache2/mods-available/cgi.load /etc/apache2/mods-enabled/cgi.load

COPY ./vinayaka.conf /etc/apache2/sites-enabled
COPY . /vinayaka

RUN chmod -R 777 /vinayaka \
 && make \
 && make install \
 && make initialize

RUN echo '\
10 */3 * * * /usr/local/bin/vinayaka-user-speed-cron\n\
20 4   * * * /usr/local/bin/vinayaka-user-avatar-cron\n\
30 5   * * * /usr/local/bin/vinayaka-user-words-meta-cron\n\
40 */4 * * * /usr/local/bin/vinayaka-model-collector-cron\n\
50 *   * * * /usr/local/bin/vinayaka-clear-cache-cron\n\
' >> /etc/crontab

ENTRYPOINT ["/usr/sbin/apache2ctl", "-D", "FOREGROUND"]
