#!/bin/bash
    # The geometry and frame rate are from your system settings in Snowmix
    width=640
    height=480
    framerate='30/1'
    which gst-launch-1.0 2>/dev/null 1>&2
    if [ $? -eq 0 ] ; then
      gstlaunch=gst-launch-1.0
      VIDEOCONVERT=videoconvert
      MIXERFORMAT='video/x-raw, format=BGRA, pixel-aspect-ratio=1/1, interlace-mode=progressive'
    else
      gstlaunch=gst-launch-0.10
      VIDEOCONVERT=ffmpegcolorspace
      MIXERFORMAT="video/x-raw-rgb,bpp=32,depth=32,endianness=4321,red_mask=65280,green_mask=16711680"
      MIXERFORMAT="$MIXERFORMAT,blue_mask=-16777216,pixel-aspect-ratio=1/1,interlaced=false"
    fi
    SRC='shmsrc socket-path=/tmp/mixer1 do-timestamp=true is-live=true'
    $gstlaunch -v              \
        $SRC                  !\
        $MIXERFORMAT", framerate=$framerate, width=$width, height=$height" !\
        queue                 !\
        $VIDEOCONVERT         !\
        queue                 !\
        jpegenc ! queue !\
        queue                 !\
        filesink location=./../Client/SnowmixScripts/videoPipe
