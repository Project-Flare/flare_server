#!/bin/sh

openssl req -nodes -new -x509 -newkey ed25519 -keyout key.pem -out cert.pem