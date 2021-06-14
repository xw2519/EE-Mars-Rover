'''
api.py

Handles the WebSocket connection interfaces of the Command website and rover.

WARNING: Database components are disabled (commented out) due to MongoDB database server not configured. Rest of the FastAPI server will work as normal.
'''

from app.database import *
from fastapi import FastAPI, Request, BackgroundTasks, WebSocket, WebSocketDisconnect
from fastapi.responses import HTMLResponse
from fastapi.middleware.cors import CORSMiddleware
import json
from datetime import datetime

app = FastAPI(title='Mars Command Server')

'''
Initate MongoDB database collection using the current time as the collection name
now = datetime.now()

current_time = now.strftime("%H:%M:%S")
Session_collection = create_database_collection("Session ", current_time)
'''

origins = [
    "http://localhost:3000"
]


app.add_middleware(
    CORSMiddleware,
    allow_origins=origins,
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

'''
Communication interfaces:

    Rover to Server - JSON:
        Map (Rover):
        - x_distance 
        - y_distance 
        - angle
        
        Map (Obstacle):
        - x_distance 
        - y_distance 
        - color  
        
        Terminal logging:
        - Message 
        
        Sensor data:
        - Battery remaining
        - Distance remaining 
        - Distance travelled 

    Server to Rover - Serial:
        - Navigation commands 
        - Automation toggle
'''

received_data = ""
received_data_rover = ""

'''
Connection management functions
'''

class ClientManager:
    '''
    Website connection functions
    '''
    def __init__(self):
        self.connection: WebSocket
    
    async def connect(self, websocket_client: WebSocket):
        await websocket_client.accept()
        self.connection = websocket_client
    
    async def send_to_client(self, data_client: str):
        await self.connection.send_text(data_client)
        
    async def disconnect(self):
        await self.connection.close()
        
class RoverManager:
    '''
    Rover connection functions
    '''
    def __init__(self):
        self.connection: WebSocket
    
    async def connect(self, websocket_client: WebSocket):
        await websocket_client.accept()
        self.connection = websocket_client
    
    async def send_to_rover(self, data_rover: str):
        await self.connection.send_text(data_rover)
            
    async def disconnect(self):
        await self.connection.close()
        
class Session:
    '''
    Session (connection instance) functions
    '''
    def __init__(self):
        self.client_connection = ClientManager()
        self.rover_connection = RoverManager()
        

'''
Server routes
'''       
        
session_instance = Session()

@app.post("/login")
async def login_handler():
    return {"token": "test234"}


@app.websocket("/ws/server")
async def websocket_client(websocket: WebSocket):
    '''
    Handle server <-> website websocket connection
    '''
    print('[Server]: Establishing command client websocket connection')
    await session_instance.client_connection.connect(websocket)
    
    try:
        while True:
            # Receive messages from command client 
            received_data = await websocket.receive_text()
            
            if (received_data != ""):
                print("[Server Info]: Sending to rover: " + received_data)              
                await session_instance.rover_connection.send_to_rover(received_data)
            
    except WebSocketDisconnect:
        print("[Server Error]: Command client websocket terminated")


@app.websocket("/ws/rover")
async def websocket_endpoint(websocket: WebSocket):
    '''
    Handle server <-> rover websocket connection
    '''
    print("[Server Error]: Establishing websocket connection with rover")
    await session_instance.rover_connection.connect(websocket)
    
    try:
        while True:
            # Receive messages from the rover 
            received_data_rover = await websocket.receive_text()
            
            if (received_data_rover != ""):
                print("[Server Info]: Sending to client: " + received_data_rover)  
                await session_instance.client_connection.send_to_client(received_data_rover)
                
                '''
                Database component: Uploads 
                    - details and coordinates of the rover and obstacles to MongoDB database server
                    - Time and tilt of the rover 
                
                # Load JSON
                data = json.loads(received_data_rover)         
                
                # Check if message is rover or obstacle data or tilt data
                if (data.type == "Map"):
                    # Upload to the database
                    insert_record(Session_collection, data)  
                     
                else if (data.type == "Tilt"):
                    # Upload to the database
                    current_time = {"time":datetime.now()}
                    data.update(current_time)
                    insert_record(Session_collection, data)   
                '''
                
    except WebSocketDisconnect:
        print("[Server Error]: Rover websocket terminated")
            