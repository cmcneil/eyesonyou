import asyncio
import datetime
import random
import websockets


async def hello(websocket, path):
    while True:
        now = datetime.datetime.utcnow().isoformat() + "Z"
        print(now)
        await websocket.send("Hello at " + now)
        await asyncio.sleep(random.random() * 3)

start_server = websockets.serve(hello, '0.0.0.0', port=8888)

asyncio.get_event_loop().run_until_complete(start_server)
asyncio.get_event_loop().run_forever()
