from fastapi import FastAPI, Request, BackgroundTasks, WebSocket
from fastapi.middleware.cors import CORSMiddleware

app = FastAPI(title='Mars Command Server')

origins = [
    "http://localhost:3000",
    "localhost:3000"
]

# Handle cross origin requests i.e. requests from frontend domain
app.add_middleware(
    CORSMiddleware,
    allow_origins=origins,
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"]
)

# Establish websocket with client website by upgrading HTTP connection
@app.websocket("/ws_server")
async def websocket_endpoint(websocket: WebSocket):
    print('[Server]: Establishing websocket connection')
    await websocket.accept()
    while True:
        data = await websocket.receive_text()
        print(data)

