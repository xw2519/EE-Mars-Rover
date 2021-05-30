from fastapi import FastAPI, Request, BackgroundTasks, WebSocket, WebSocketDisconnect
from fastapi.responses import HTMLResponse
from fastapi.middleware.cors import CORSMiddleware
from typing import List
import json
import itertools as it

app = FastAPI(title='Mars Command Server')

'''
Server to web client - JSON:
    - Battery remaining 
    - Distance remaining 
    - Distance travelled 
    - Map coordinates

Web client to server - JSON:
    - Functions 
    - Logging entry
'''
received_data: str

# Client based functions
class ClientManager:
    def __init__(self):
        self.connection: WebSocket
    
    async def connect(self, websocket_client: WebSocket):
        print('Executing')
        await websocket_client.accept()
        self.connection = websocket_client
    
    async def send_to_client(self, data: str):
        await self.connection.send_text(data)
        
# Rover based functions
class RoverManager:
    def __init__(self):
        self.connection: WebSocket
    
    async def connect(self, websocket_client: WebSocket):
        await websocket_client.accept()
        self.connection = websocket_client
    
    async def send_to_rover(self, data: str):
        await self.connection.send_text(data)
        
# Session based functions
class Session:
    def __init__(self):
        self.client_connection = ClientManager()
        self.rover_connection = RoverManager()

        
'''
Main server operations
'''       
        
session_instance = Session()

@app.websocket("/ws/server")
async def websocket_client(websocket: WebSocket):
    print('[Server]: Establishing command client websocket connection')
    await session_instance.client_connection.connect(websocket)
    
    try:
        while True:
            received_data = await websocket.receive_text()
            await session_instance.rover_connection.send_to_rover(received_data)
            print(received_data)
            
    except WebSocketDisconnect:
        print('[Server]: Command client websocket connection terminated')

@app.websocket("/ws/rover")
async def websocket_endpoint(websocket: WebSocket):
    print('[Server]: Establishing websocket connection with rover')
    await session_instance.rover_connection.connect(websocket)
    
    try:
        while True:
            data = await websocket.receive_text()
            print(received_data)
            
            await websocket.send_text(received_data)
            
    except WebSocketDisconnect:
        print('[Server]: Command client websocket connection terminated')
        
    