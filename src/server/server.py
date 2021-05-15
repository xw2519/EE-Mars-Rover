""" This file defines how the server part of the backend functions """

# Import server packages
from os import environ
from json import dumps 
from dotenv import load_dotenv 
from fastapi import FastAPI, Request, BackgroundTasks
from fastapi.templating import Jinja2Templates
from socketio import *

# Server core setting
load_dotenv()
HOST = environ.get("HOST")
PORT = int(environ.get("PORT"))
URL = "ws://" + HOST + ":" + str(PORT)
FAPI = FastAPI()
Web_template = Jinja2Templates(directory="")




