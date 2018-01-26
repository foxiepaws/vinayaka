#!/bin/sh
/usr/sbin/cron && /usr/sbin/apache2ctl -D FOREGROUND
