#!/usr/bin/python
#
# Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
#

import asyncio
import websockets

import jetson.inference
import jetson.utils
import pyrealsense2 as rs
import numpy as np

import argparse
import sys

USE_DISPLAY = False

# parse the command line
parser = argparse.ArgumentParser(description="Locate objects in a live camera stream using an object detection DNN.", 
						   formatter_class=argparse.RawTextHelpFormatter, epilog=jetson.inference.detectNet.Usage())

parser.add_argument("--network", type=str, default="ssd-mobilenet-v2", help="pre-trained model to load (see below for options)")
parser.add_argument("--overlay", type=str, default="box,labels,conf", help="detection overlay flags (e.g. --overlay=box,labels,conf)\nvalid combinations are:  'box', 'labels', 'conf', 'none'")
parser.add_argument("--threshold", type=float, default=0.5, help="minimum detection threshold to use") 
parser.add_argument("--camera", type=str, default="0", help="index of the MIPI CSI camera to use (e.g. CSI camera 0)\nor for VL42 cameras, the /dev/video device to use.\nby default, MIPI CSI camera 0 will be used.")
parser.add_argument("--width", type=int, default=1280, help="desired width of camera stream (default is 1280 pixels)")
parser.add_argument("--height", type=int, default=720, help="desired height of camera stream (default is 720 pixels)")

try:
	opt = parser.parse_known_args()[0]
except:
	print("")
	parser.print_help()
	sys.exit(0)

# load the object detection network
net = jetson.inference.detectNet(opt.network, sys.argv, opt.threshold)

# create the camera and display
# ~ camera = jetson.utils.gstCamera(opt.width, opt.height, opt.camera)
if USE_DISPLAY:
	display = jetson.utils.glDisplay()

pipe = rs.pipeline()

config = rs.config()
config.enable_stream(rs.stream.depth, 640, 480, rs.format.z16, 30)
config.enable_stream(rs.stream.color, 640, 480, rs.format.rgb8, 30)
profile = pipe.start(config)

depth_sensor = profile.get_device().first_depth_sensor()
depth_scale = depth_sensor.get_depth_scale()
print("Depth Scale is: " , depth_scale)

# We will be removing the background of objects more than
#  clipping_distance_in_meters meters away
clipping_distance_in_meters = 1 #1 meter
clipping_distance = clipping_distance_in_meters / depth_scale

# Create an align object
# rs.align allows us to perform alignment of depth frames to others frames
# The "align_to" is the stream type to which we plan to align depth frames.
align_to = rs.stream.color
align = rs.align(align_to)

rgba = np.zeros((480, 640, 4))
rgba[:, :, 3] = 255

# process frames until user exits
current_depth_estimate = 0.0
try:
	# ~ while display.IsOpen():
		# capture the image
		# ~ img, width, height = camera.CaptureRGBA()
	while True:
		frames = pipe.wait_for_frames()
		aligned_frames = align.process(frames)
		rgb_frame = aligned_frames.get_color_frame()
		depth_frame = aligned_frames.get_depth_frame()
		if not rgb_frame or not depth_frame:
			continue
		
		width = int(rgb_frame.get_width())
		height = int(rgb_frame.get_height())
		rgba[:, :, 0:3] = rgb_frame.get_data()
		img = jetson.utils.cudaFromNumpy(rgba)
		
		# detect objects in the image (with overlay)
		detections = net.Detect(img, width, height, opt.overlay)
		
		# print the detections
		# ~ print("detected {:d} objects in image".format(len(detections)))
		
		person_detection = None
		for detection in detections:
			if detection.ClassID == 1:
				# ~ print(detection)
				print(detection.Center)
				person_detection = detection
		if person_detection is not None:
			x, y = person_detection.Center
			# ~ norm_point = (x / depth_frame.getWidth(), y / depth_frame.getWidth())
			# ~ depth_points = 
			current_depth_estimate = depth_frame.get_distance(int(x), int(y))
			print('Updated Depth Estimate to: {}'.format(current_depth_estimate))
		if USE_DISPLAY:
			# render the image
			display.RenderOnce(img, width, height)
			
			# update the title bar
			display.SetTitle("{:s} | Network {:.0f} FPS".format(opt.network, net.GetNetworkFPS()))
		
		# print out performance info
		# ~ net.PrintProfilerTimes()
finally:
	pipe.stop()

