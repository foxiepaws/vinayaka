#!/bin/bash
echo 'Content-Type: application/json'
echo ''
/usr/local/bin/vinayaka-user-match-impl "$1" "$2" 20

