import speech_recognition as sr
import os
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
            who_for_send = await websocket.recv() #? In this Line is status from client ["E1","E2"]
            message,status_for_convert = speech_to_text("resource",who_for_send)
            
            #! Prepare Process Change Sound to Text
            for receiver_ws in CONNECTIONS:
                if receiver_ws != websocket:
                    await receiver_ws.send(message)
                    delete_file_aftersend()
                    continue
                print(f"{datetime.datetime.now()} {message}") #. Log for server
    finally:
        CONNECTIONS.remove(websocket)

async def server():

    async with websockets.serve(handler, "0.0.0.0", 34567, ping_interval=None):
        await asyncio.Future()

asyncio.run(server())

def check_voice_file():
    for item in os.listdir("resource"):
        if item 
    
def delete_file_aftersend(file_name):
    try:
        os.remove("resource/recording"+file_name+".wav")
        print("Delete file successful")
        return True
    except FileNotFoundError:
        print("File Not Found")
        return False
    except Exception as e:
        print(e,"This is Error")
        return False
        
def speech_to_text(path,file_name):

    final_path = path+"/"+"recoding"+file_name+".wav"

    with sr.AudioFile(final_path) as source:
        audio_text = Recognize.listen(source)

        try:
            text = Recognize.recognize_google(audio_text)
            status = True
        except:
            text = "Please Try again"
            status = False

    return text,status
    