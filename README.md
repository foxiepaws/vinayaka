# Vinayaka

語彙の類似からマストドンのユーザーを推挙するウェブアプリケーション

http://vinayaka.distsn.org

## API

http://vinayaka.distsn.org/cgi-bin/vinayaka-user-match-api.cgi?mstdn.jp+nullkal

UTF-8 の JSON が返ります。

## Install

(Recommends Ubuntu)

$ sudo apt install git make gcc g++ libcurl4-openssl-dev apache2

$ git clone https://github.com/distsn/vinayaka.git

$ cd vinayaka

$ make

$ sudo make install

$ sudo make initialize

$ sudo make activate-cgi

$ crontab -e

Write following code in crontab:

    10 */3 * * * /usr/local/bin/vinayaka-user-speed-cron
    20 4   * * * /usr/local/bin/vinayaka-user-avatar-cron
    30 */6 * * * /usr/local/bin/vinayaka-user-words-meta-cron
    40 */3 * * * /usr/local/bin/vinayaka-model-collector-cron
    50 *   * * * /usr/local/bin/vinayaka-clear-cache-cron

# Update

$ cd vinayaka

$ git pull

$ make clean

$ make

$ sudo make install
