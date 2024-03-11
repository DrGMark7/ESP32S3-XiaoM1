import speech_recognition as sr
import os
import asyncio
import datetime
import websockets
from websockets.legacy.server import WebSocketServerProtocol

print("======== GET START SERVER PYTHON =========")

CONNECTIONS: list[WebSocketServerProtocol] = []
Recognize = sr.Recognizer()
def check_voice_file(file_name):

    for item in os.listdir("resources"):
        key_check= item.split(".")[0][-2:]
        if key_check == file_name:
            return True
        
    return False
    
def delete_file_aftersend(file_name):
    try:
        os.remove("resources/recording"+file_name+".wav")
        print("[LOG] : Delete file successful")
        return True
    except FileNotFoundError:
        print("[LOG] : File Not Found")
        return False
    except Exception as e:
        print("[LOG] : ",e,"This is Error")
        return False
    
def speech_to_text(path,file_name):

    final_path = path+"/"+"recording"+file_name+".wav"
    with sr.AudioFile(final_path) as source:
        audio_text = Recognize.listen(source)

        try:
            text = Recognize.recognize_google(audio_text)
            status = True
            print(f"[LOG] : S2T {status}")
        except:
            text = "Please Try again"
            status = False
            print(f"[LOG] : S2T {status}")

    return text,status

async def handler(websocket: WebSocketServerProtocol):
    CONNECTIONS.append(websocket)
    try:
        while True:
            #! Client Send some siganificant for status
            who_send = await websocket.recv() #? In this Line is status from client ["E1","E2"]
            
            if check_voice_file(who_send):
                print("[LOG] : Voice file found")
            else:
                print("[LOG] : Voice not found")
                break
            
            message,status_for_convert = speech_to_text("resources",who_send)
            
            #! Prepare Process Change Sound to Text
            for receiver_ws in CONNECTIONS:
                if receiver_ws != websocket:
                    await receiver_ws.send(" "+message+" ")
                    if status_for_convert:
                        pass
                        # delete_file_aftersend(who_send)
                    continue

                print(f"{str(datetime.datetime.now())[:-7]} {message}") #. Log for server
    finally:
        CONNECTIONS.remove(websocket)

async def server():

    async with websockets.serve(handler, "0.0.0.0", 34567, ping_interval=None):
        await asyncio.Future()

asyncio.run(server())


    