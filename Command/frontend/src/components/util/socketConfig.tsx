// Establish websocket connection with server

const ws_server = new WebSocket("ws://18.218.68.104:8000/ws/server"); 

//const ws_server = new WebSocket("ws://localhost:8000/ws/server"); 

export default ws_server;