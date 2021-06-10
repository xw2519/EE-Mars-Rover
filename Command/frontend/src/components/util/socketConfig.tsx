// Establish websocket connection with server

const ws_server = new WebSocket("ws://3.16.40.157:8000/ws/server"); 

//const ws_server = new WebSocket("ws://localhost:8000/ws/server"); 

export default ws_server;