// Establish websocket connection with server

const server_IP_address:string = "18.216.32.177"

const ws_server = new WebSocket("ws://" + server_IP_address + ":8000/ws/server"); 

//const ws_server = new WebSocket("ws://localhost:8000/ws/server"); 

export default ws_server;

export {server_IP_address};