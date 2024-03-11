import asyncio
import datetime
import websockets

from websockets.legacy.server import WebSocketServerProtocol

print("======== GET START SERVER PYTHON =========")

CONNECTIONS: list[WebSocketServerProtocol] = []

async def handler(websocket: WebSocketServerProtocol):
    CONNECTIONS.append(websocket)
    try:
        while True:
            message = await websocket.recv()
            # print(message)
            for receiver_ws in CONNECTIONS:
                if receiver_ws != websocket:
                    await receiver_ws.send(" "+message+" ")
                    continue
                print(f"{str(datetime.datetime.now())[:-7]} {message}")
    finally:
        CONNECTIONS.remove(websocket)

async def server():

    async with websockets.serve(handler, "0.0.0.0", 34567, ping_interval=None):
        await asyncio.Future()

asyncio.run(server())