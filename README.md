# Vinayaka

語彙の類似からマストドンのユーザーを推挙するウェブアプリケーション

http://vinayaka.distsn.org
(Alias: http://mastodonusermatching.tk)

## API

いずれもUTF-8 の JSON が返ります。

### 似ているユーザーを検索

http://vinayaka.distsn.org/cgi-bin/vinayaka-user-match-simple-api.cgi?mstdn.jp+nullkal (軽量)

http://vinayaka.distsn.org/cgi-bin/vinayaka-user-match-api.cgi?mstdn.jp+nullkal (フル)

### 流速順ユーザーリスト

http://vinayaka.distsn.org/cgi-bin/vinayaka-user-speed-api.cgi

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
    30 5   * * * /usr/local/bin/vinayaka-user-words-meta-cron
    40 */4 * * * /usr/local/bin/vinayaka-model-collector-cron
    50 *   * * * /usr/local/bin/vinayaka-clear-cache-cron

# Update

    $ cd vinayaka
    $ git pull
    $ make clean
    $ make
    $ sudo make install
