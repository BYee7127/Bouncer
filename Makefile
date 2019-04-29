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

# 3rd test for a different file
test3:
	./bouncer coffee2.jpg

# 4th test for a larger file
test4:
	./bouncer coffee.jpg

# removes all files
clean:
# the -f makes the clean succeed even if the files don't exist
	rm -f *.cool bouncer

# makes the movie out of the .cool files
movie:
	ffmpeg -r 30 -i frame%03d.cool movie.mp4

# plays the movie
play:
	ffplay movie.mp4