import React from 'react';
import ws_server from '../components/socketConfig'

// Add in angle and stuffies 

class NavUI extends React.Component {
    NavUp = () => {
        console.log("UP");

        // Websocket communication 
        ws_server.send("UP");
    }

    NavDown = () => {
        console.log("DOWN");

        // Websocket communication 
        ws_server.send("DOWN");
    }

    NavRight = () => {
        console.log("RIGHT");

        // Websocket communication 
        ws_server.send("RIGHT");
    }

    NavLeft = () => {
        console.log("LEFT");

        // Websocket communication 
        ws_server.send("LEFT");
    }

    NavHalt = () => {
        console.log("HALT");

        // Websocket communication 
        ws_server.send("HALT");
    }
    
    /* Serve landing page */
    render() {
        
        return (
    
            <div className="NavButtons">
                <div className="Top"> 
                    <button className = "Up" onClick={this.NavUp}> Up </button>
                </div>
                <div className="Middle"> 
                    <button className = "Left" onClick={this.NavLeft}> Left </button>
                    <button className = "Halt" onClick={this.NavHalt}> Halt </button>
                    <button className = "Right" onClick={this.NavRight}> Right </button>
                </div>
                <div className="Bottom"> 
                    <button className = "Down" onClick={this.NavDown}> Down </button>
                </div>
            </div>

        );
    }
}

export default NavUI;