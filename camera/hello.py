import pyrealsense2 as rs

pipe = rs.pipeline()
profile = pipe.start()

try:
    while True:
        frames = pipe.wait_for_frames()
        depth = frames.get_depth_frame()

        width = int(depth.get_width())
        height = int(depth.get_height())
#        print(type(depth))
#        print(type(width))
        dist_to_center = depth.get_distance(width // 2, height // 2)

        print("The camera is facing an object {} meters away".format(
            dist_to_center))
finally:
    pipe.stop()
