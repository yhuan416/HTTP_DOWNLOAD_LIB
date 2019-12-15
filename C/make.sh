#!/bin/sh

CC=gcc

$CC main.c -L ../lib -lHTTPDown -lpthread -o test 



