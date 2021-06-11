from fastapi import FastAPI, Request, BackgroundTasks, WebSocket, WebSocketDisconnect
from fastapi.responses import HTMLResponse
from fastapi.middleware.cors import CORSMiddleware
import json

app = FastAPI(title='Mars Command Server')

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
Server to web client - JSON:
    - Battery remaining 
    - Distance remaining 
    - Distance travelled 
    - Map coordinates

Web client to server - JSON:
    - Functions 
    - Logging entry
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
        try:
            await self.connection.send_text(data_client)
        except:
            print("[Server Error]: No command client is connected to the system") 
        
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
        try:
            await self.connection.send_text(data_rover)
        except:
            print("[Server Error]: No rover is connected to the system") 
        
class Session:
    '''
    Session (connection instance) functions
    '''
    def __init__(self):
        self.client_connection = ClientManager()
        self.rover_connection = RoverManager()
        

'''
Main server operations
'''       
        
session_instance = Session()

@app.post("/login")
async def login_handler():
    return {"token": "test123"}


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
            
            # Testing
            terminal_check = {
                "type" : "Map",
                "x_distance" : "0.00",
                "y_distance" : "80",
                "map_type" : "Rover",
                "angle": "89.90"
                }
                
            json_object = json.dumps(terminal_check)
            
            await session_instance.client_connection.send_to_client(json_object)
            
            terminal_check = {
                "type" : "Map",
                "x_distance" : "100.00",
                "y_distance" : "80",
                "map_type" : "Rover",
                "angle": "89.90"
                }
                
            json_object = json.dumps(terminal_check)
            
            await session_instance.client_connection.send_to_client(json_object)
            
            terminal_check = {
                "type" : "Map",
                "x_distance" : "140.00",
                "y_distance" : "140",
                "map_type" : "Obstacle",
                "color": "red"
                }
                
            json_object = json.dumps(terminal_check)
            
            await session_instance.client_connection.send_to_client(json_object)

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

    except WebSocketDisconnect:
        print("[Server Error]: Rover websocket terminated")

    