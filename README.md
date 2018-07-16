# Vinayaka

語彙の類似からマストドンのユーザーを推挙するウェブアプリケーション

http://vinayaka.distsn.org
(Alias: http://mastodonusermatching.tk)

## API

いずれもUTF-8 の JSON が返ります。

### 似ているユーザーを検索

http://vinayaka.distsn.org/cgi-bin/vinayaka-user-match-api.cgi?mastodon.social+Gargron  
(フル)

http://vinayaka.distsn.org/cgi-bin/vinayaka-user-match-filtered-api.cgi?mastodon.social+Gargron  
(本人、フォロー済み、ブラックリスト、ボットを除く)

http://vinayaka.distsn.org/cgi-bin/vinayaka-user-match-osa-api.cgi?mastodon.social+Gargron  
([おすすめフォロワー](https://followlink.osa-p.net/recommend.html)互換形式)

http://vinayaka.distsn.org/cgi-bin/vinayaka-user-match-suggestions-api.cgi?mastodon.social+Gargron  
(マストドン公式 `/api/v1/suggestions` 互換)

### ユーザーを検索

ユーザー名、スクリーンネーム、bioを検索します。

http://vinayaka.distsn.org/cgi-bin/vinayaka-user-search-api.cgi?nagiept

### 新規ユーザーリスト

http://vinayaka.distsn.org/cgi-bin/vinayaka-user-new-api.cgi

### 流速順ユーザーリスト

http://vinayaka.distsn.org/cgi-bin/vinayaka-user-speed-api.cgi?100 (上位n人)

http://vinayaka.distsn.org/cgi-bin/vinayaka-user-speed-api.cgi (フル)

## Install

    $ sudo apt install build-essential libcurl4-openssl-dev apache2
    $ make
    $ sudo make install
    $ sudo make initialize
    $ sudo make activate-cgi
    $ crontab -e

Write following code in crontab:

    10 */3 * * * /usr/local/bin/vinayaka-user-speed-cron
    0  */2 * * * /usr/local/bin/vinayaka-user-new-cron
    30 */6 * * * /usr/local/bin/vinayaka-user-avatar-cron
    */15 * * * * /usr/local/bin/vinayaka-clear-cache-cron
    20 */6 * * * /usr/local/bin/vinayaka-collect-raw-toots-cron
    50 */6 * * * /usr/local/bin/vinayaka-store-raw-toots-cron

Write following code in crontab for the root:

    42 2 * * 2 /usr/local/bin/vinayaka-https-renew-cron

## Update

    $ make clean
    $ make
    $ sudo make install
