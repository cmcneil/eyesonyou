import jetson.inference
import jetson.utils
import pyrealsense2 as rs
import numpy as np
import matplotlib.cm

import argparse
import sys

# parse the command line
parser = argparse.ArgumentParser(description="Classify a live camera stream using an image recognition DNN.", 
						   formatter_class=argparse.RawTextHelpFormatter, epilog=jetson.inference.imageNet.Usage())

parser.add_argument("--network", type=str, default="googlenet", help="pre-trained model to load (see below for options)")
parser.add_argument("--camera", type=str, default="0", help="index of the MIPI CSI camera to use (e.g. CSI camera 0)\nor for VL42 cameras, the /dev/video device to use.\nby default, MIPI CSI camera 0 will be used.")
parser.add_argument("--width", type=int, default=1280, help="desired width of camera stream (default is 1280 pixels)")
parser.add_argument("--height", type=int, default=720, help="desired height of camera stream (default is 720 pixels)")

pipe = rs.pipeline()
profile = pipe.start()

# open the camera for streaming
# ~ camera.Open()
# ~ rgba = np.zeros((480, 640, 4))


try:
	opt = parser.parse_known_args()[0]
except:
	print("")
	parser.print_help()
	sys.exit(0)

# load the recognition network
net = jetson.inference.imageNet(opt.network, sys.argv)

# create the camera and display
font = jetson.utils.cudaFont()
# ~ camera = jetson.utils.gstCamera(opt.width, opt.height, opt.camera)
display = jetson.utils.glDisplay()

# Get Camera Depth
color_retrieved = False
depth_retrieved = False
color_dims = None
depth_dims = None
while not (color_retrieved and depth_retrieved):
	frames = pipe.wait_for_frames()
	rgb_frame = frames.get_color_frame()
	depth_frame = frames.get_depth_frame()
	if not color_retrieved and rgb_frame:
		color_dims = (rgb_frame.get_width(), rgb_frame.get_height())
		color_retrieved = True
	if not depth_retrieved and depth_frame:
		depth_dims = (depth_frame.get_width(), depth_frame.get_height())
		depth_retrieved = True

# ~ disp_buff = np.zeros((max(color_dims[1], depth_dims[1]),
					  # ~ color_dims[0]+depth_dims[0],
					  # ~ 4))
# ~ disp_buff[:, :, 3] = 255
# ~ depth_tmp = np.zeros((depth_dims[1], depth_dims[0]))

# ~ full_w = np.shape(disp_buff)[1]
# ~ full_h = np.shape(disp_buff)[0]

color_only_buff = np.zeros((color_dims[1], color_dims[0], 4))
color_only_buff[:, :, 3] = 255

depth_cmap = matplotlib.cm.get_cmap('viridis')
# process frames until user exits
current_depth = 0
while display.IsOpen():
	# capture the image
	# ~ img, width, height = camera.CaptureRGBA()
	frames = pipe.wait_for_frames()
	rgb_frame = frames.get_color_frame()
	depth_frame = frames.get_depth_frame()
	if not (rgb_frame):
		continue
	# ~ if rgb_frame:
		# ~ disp_buff[0:color_dims[1], 0:color_dims[0], 0:3] = rgb_frame.get_data()
	color_only_buff[:, :, 0:3] = rgb_frame.get_data()
	if depth_frame:
		width = int(depth_frame.get_width())
		height = int(depth_frame.get_height())
		current_depth = depth_frame.get_distance(width // 2, height // 2)
		print('Current Depth: {}'.format(current_depth))
		# ~ print(depth_tmp[300, 300])
		# ~ disp_buff[0:depth_dims[1], color_dims[0]:(color_dims[0]+depth_dims[0]), :] = (depth_cmap(depth_tmp / 10.)*255).astype(int)
	
	# ~ print(rgba)
	cuda_buff = jetson.utils.cudaFromNumpy(color_only_buff)
	# ~ cuda_color_im = jetson.utils.cudaFromNumpy(disp_buff[0:color_dims[1], 0:color_dims[0], :])
	# classify the image
	class_idx, confidence = net.Classify(cuda_buff, color_dims[0], color_dims[1])

	# find the object description
	class_desc = net.GetClassDesc(class_idx)

	# overlay the result on the image	
	font.OverlayText(cuda_buff, color_dims[0], color_dims[1],
					 "{:05.2f}% {:s}".format(confidence * 100, class_desc), 5, 5, font.White, font.Gray40)
	
	# render the image
	display.RenderOnce(cuda_buff, color_dims[0], color_dims[1])

	# update the title bar
	display.SetTitle("{:s} | Network {:.0f} FPS | Depth: {}".format(net.GetNetworkName(), net.GetNetworkFPS(),
																	current_depth))

	# print out performance info
	# ~ net.PrintProfilerTimes()
