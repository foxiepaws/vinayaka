#!/bin/bash

echo 'Access-Control-Allow-Origin: *'
echo 'Content-Type: application/json'
echo ''
cat /var/lib/vinayaka/users-new-cache.json

