# Vinayaka

語彙の類似からマストドンのユーザーを推挙するウェブアプリケーション

http://vinayaka.distsn.org

## API

http://vinayaka.distsn.org/cgi-bin/vinayaka-user-match-api.cgi?mstdn.jp+nullkal

UTF-8 の JSON が返ります。

## Install

(Recommends Ubuntu)

$ sudo apt install git make gcc g++ libcurl4-openssl-dev apache2

$ mkdir -p git

$ cd git

$ git clone https://github.com/distsn/vinayaka.git

$ cd vinayaka

$ make

$ sudo make install

$ sudo make initialize

$ sudo make activate-cgi

$ crontab -e

Write following code in crontab:

0 */6 * * * /home/ubuntu/git/vinayaka/server/vinayaka-user-words-meta-cron

# Update

$ cd ~/git/vinayaka

$ git pull

$ make clean

$ make

$ sudo make install
