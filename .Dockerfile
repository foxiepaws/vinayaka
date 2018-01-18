FROM alpine:3.7

WORKDIR /vinayaka

RUN apk -U upgrade \
 && apk add \
    git \
    make \
    gcc \
    g++ \
    libcurl4-openssl-dev \
    apache2

RUN make \
 && make install \
 && make initialize \
 && make activate-cgi \
 && crontab -e

RUN echo '\\
    10 */3 * * * /usr/local/bin/vinayaka-user-speed-cron\\
    20 4   * * * /usr/local/bin/vinayaka-user-avatar-cron\\
    30 5   * * * /usr/local/bin/vinayaka-user-words-meta-cron\\
    40 */4 * * * /usr/local/bin/vinayaka-model-collector-cron\\
    50 *   * * * /usr/local/bin/vinayaka-clear-cache-cron\\
    ' >> /etc/crontab

CMD ["cron", "-f"]
