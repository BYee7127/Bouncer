# Makefile for the bouncer application
#
# Author:   Kade Challis, Beverly Yee
# Date:     April 09, 2019

# compiles the bouncer.cpp file - target: dependencies
# bouncer is the output file that depends on bouncer.cpp
# It is the default rule that rebuilds the bouncer application
all: bouncer.cpp
	g++ bouncer.cpp `pkg-config --cflags --libs libavcodec libavutil libavformat libswscale` -o bouncer

# tests the bouncer file to make sure everything is working correctly
test:
	./bouncer candy.jpg

# 2nd test to see if the extension checker is working
test2:
	./bouncer ../Downloads/marbles2.bmp

# removes all files
clean:
# the -f makes the clean succeed even if the files don't exist
	rm -f *.cool bouncer
