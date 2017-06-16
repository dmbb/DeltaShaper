#!/bin/bash

Xvfb :1 -screen 0 1280x1024x24 &
sudo ffmpeg -nostats -t 8 -re -i coverVideo.mp4 -r 30 -vcodec rawvideo -pix_fmt yuv420p -threads 0 -f v4l2 /dev/video0 & 
sleep 3
DISPLAY=:1.0 skype &
#sleep 3
#python autoAnswer.py
