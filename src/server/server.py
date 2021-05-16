""" 
    This file defines how the server part of the backend functions. 
    
    Upon execution, sets up a server on the specified IP and PORT defined in ".env" file.
"""

# Import server packages
from os import environ
from json import dumps 
from dotenv import load_dotenv 
from fastapi import FastAPI, Request, BackgroundTasks
from fastapi.templating import Jinja2Templates
from socketio import *
from uvicorn import run

# Server core settings
load_dotenv()
HOST = environ.get("HOST")
PORT = int(environ.get("PORT"))
URL = "ws://" + HOST + ":" + str(PORT)
app = FastAPI()
templates = Jinja2Templates(directory="templates")

sio = AsyncServer(async_mode="asgi", cors_allowed_origins="*")
socket_app = ASGIApp(sio)


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

if __name__ == "__main__":
    run('server:app', host=HOST, port=PORT)