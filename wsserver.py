import speech_recognition as sr
import asyncio
import datetime
import websockets
from websockets.legacy.server import WebSocketServerProtocol

print("======== GET START SERVER PYTHON =========")

CONNECTIONS: list[WebSocketServerProtocol] = []
Recognize = sr.Recognizer()


async def handler(websocket: WebSocketServerProtocol):
    CONNECTIONS.append(websocket)
    try:
        while True:
            #! Client Send some siganificant for status
            message = await websocket.recv()
            # print(message)
            #! Prepare Process Change Sound to Text
            for receiver_ws in CONNECTIONS:
                if receiver_ws != websocket:
                    await receiver_ws.send(message)
                    continue
                print(f"{datetime.datetime.now()} {message}")
    finally:
        CONNECTIONS.remove(websocket)

async def server():

    async with websockets.serve(handler, "0.0.0.0", 34567, ping_interval=None):
        await asyncio.Future()

asyncio.run(server())
