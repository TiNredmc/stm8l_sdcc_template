#!/bin/bash

#This will run the "sdasstm8" before run "Make All". To make the .rel file for linking the fast_memcpy() to main.asm

sdasstm8 -lo fastmemcpy.s

make all
