#!/bin/bash

if test $(ps ax | fgrep vinayaka-user-match-impl | wc -l) -lt 2
then
	/usr/local/bin/vinayaka-user-match-impl "$1" "$2"
else
	echo 'Content-Type: application/json'
	echo ''
	echo '"ただいま混み合っております。数分お待ちいただいてからご利用ください。"'
fi

