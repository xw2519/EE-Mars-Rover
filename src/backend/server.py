""" 
    This file defines how the server part of the backend functions. 
    
    Upon execution, sets up a server on the specified IP and PORT defined in ".env" file.
"""

# Import server packages
from os import environ
from json import dumps 
from dotenv import load_dotenv 

from fastapi import FastAPI, Request, BackgroundTasks, WebSocket
from fastapi.templating import Jinja2Templates
from fastapi.staticfiles import StaticFiles

from socketio import *
from uvicorn import run

import random

# Server core settings
load_dotenv()
HOST = environ.get("HOST")
PORT = int(environ.get("PORT"))
URL = "ws://" + HOST + ":" + str(PORT)
app = FastAPI(title='Mars Command Server')

sio = AsyncServer(async_mode="asgi", cors_allowed_origins="*")
socket_app = ASGIApp(sio)

# Establish websocket connection by upgrading HTTP
@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    print('[Server]: Establishing websocket connection')
    await websocket.accept()
    while True:
        try:
            # Wait for any message from the client
            await websocket.receive_text()
            # Send message to the client
            resp = {'value': random.uniform(0, 1)}
            await websocket.send_json(resp)
        except Exception as e:
            print('[Server]: Server error ', e)
            break
    print('[Server]: Client disconnected from Command server')

# Serve website including external CSS and JS scripts
app.mount(
    "/static",
    StaticFiles(directory= "static"),
    name="static",
)

templates = Jinja2Templates(directory="templates")

@app.get("/")
def root(request: Request):
    """ Serves the rover command page """
    return templates.TemplateResponse("index.html", {"request": request, "url": URL})

"""
Socket.IO event handlers

@sio.on("connect")
def connect(sid, environ):
    print(f"Client connected, {sid}")


@sio.on("disconnect")
def disconnect(sid):
    print(f"Client disconnected, {sid}")

app.mount("/", socket_app)

"""

# Run server
if __name__ == "__main__":
    run('server:app', host=HOST, port=PORT)