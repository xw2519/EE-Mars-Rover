// Establish websocket connection with server
// 'socketConfig' support connection to localhost and to a designated server
// If connecting to a designated server, comment out line 7
// If connecting locally, comment out line 6

// const server_IP_address:string = "18.216.32.177"
const server_IP_address:string = "localhost"

const ws_server = new WebSocket("ws://" + server_IP_address + ":8000/ws/server"); 

export default ws_server;

export {server_IP_address};