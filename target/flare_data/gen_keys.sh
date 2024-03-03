#!/bin/sh

openssl ecparam -name prime256v1 -genkey -noout -out key.pem
openssl req -new -key key.pem -x509 -nodes -out cert.pem