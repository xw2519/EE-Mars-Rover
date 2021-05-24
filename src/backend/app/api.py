from fastapi import FastAPI, Request, BackgroundTasks, WebSocket
from fastapi.middleware.cors import CORSMiddleware

app = FastAPI(title='Mars Command Server')

# Establish websocket with clients by upgrading HTTP connection
@app.websocket("/ws/server")
async def websocket_endpoint(websocket: WebSocket):
    print('[Server]: Establishing websocket connection with command')
    await websocket.accept()
    while True:
        data = await websocket.receive_text()
        print(data)

@app.websocket("/ws/rover")
async def websocket_endpoint(websocket: WebSocket):
    print('[Server]: Establishing websocket connection with command')
    await websocket.accept()
    while True:
        data = await websocket.receive_text()
        print(data)
        