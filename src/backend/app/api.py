from fastapi import FastAPI, Request, BackgroundTasks, WebSocket
from fastapi.responses import HTMLResponse
from fastapi.middleware.cors import CORSMiddleware
from typing import List

app = FastAPI(title='Mars Command Server')

# Client based functions
class ClientManager:
    def __init__(self):
        self.connection: WebSocket
    
    async def connect(self, websocket: WebSocket):
        await websocket.accept()
        self.connection = websocket
        
    async def send_battery_value(self, battery_value: str):
        # Send the value of battery remaining to client panel
        
        await self.connection.send_text("50")
            
        # print("sending " + battery_value)
            

Client = ClientManager()

# Establish websocket with clients by upgrading HTTP connection
@app.websocket("/ws/server")
async def websocket_endpoint(websocket: WebSocket):
    print('[Server]: Establishing websocket connection with command')
    await Client.connect(websocket)
    while True:
        data = await websocket.receive_text()
        await Client.send_battery_value('50')
        print(data)



@app.websocket("/ws/rover")
async def websocket_endpoint(websocket: WebSocket):
    print('[Server]: Establishing websocket connection with command')
    await websocket.accept()
    while True:
        data = await websocket.receive_text()
        print(data)
        