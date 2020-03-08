import asyncio
import datetime
import json
import math
import random
import time
import websockets

tstep = 0.02 # s

def get_pos_from_time(t):
    """circle trajectory"""
    PERIOD = 3 # seconds
    R = 1 # meters
    x = R*math.cos(2*math.pi*t / PERIOD)
    y = R*math.sin(2*math.pi*t / PERIOD)
    z = 5 # meters
    return {'xyz': [x, y, z]}

async def hello(websocket, path):
    start_time = time.time()
    while True:
        t = time.time() - start_time
        pos = get_pos_from_time(t)
        # print(now)
        await websocket.send(json.dumps(pos))
        await asyncio.sleep(tstep)

start_server = websockets.serve(hello, '0.0.0.0', port=8888)

asyncio.get_event_loop().run_until_complete(start_server)
asyncio.get_event_loop().run_forever()
