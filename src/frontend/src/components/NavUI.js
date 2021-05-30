import React from 'react';
import ws_server from '../components/socketConfig'

// Add in angle and stuffies 

class NavUI extends React.Component {
    NavUp = () => {
        console.log("UP");
        ws_server.send("UP");
    }

    NavDown = () => {
        console.log("DOWN");
        ws_server.send("DOWN");
    }

    NavRight = () => {
        console.log("RIGHT");
        ws_server.send("RIGHT");
    }

    NavLeft = () => {
        console.log("LEFT");
        ws_server.send("LEFT");
    }

    NavHalt = () => {
        console.log("HALT");
        ws_server.send("HALT");
    }

    Start = () => {
        console.log("START");
        ws_server.send("START");
    }
    
    /* Serve landing page */
    render() {
        
        return (
    
            <div className="NavButtons">
                <div className="Utility"> 
                    <button className = "Start" onClick={this.Start}> Start </button>
                    <button className = "Boost" onClick={this.Start}> Boost </button>
                </div>

                <div className="Top"> 
                    <button className = "Left-Up" onClick={this.NavUp}> Left-Up </button>
                    <button className = "Up" onClick={this.NavUp}> Up </button>
                    <button className = "Right-Up" onClick={this.NavUp}> Right-Up </button>
                </div>
                <div className="Middle"> 
                    <button className = "Left" onClick={this.NavLeft}> Left </button>
                    <button className = "Halt" onClick={this.NavHalt}> Halt </button>
                    <button className = "Right" onClick={this.NavRight}> Right </button>
                </div>
                <div className="Bottom"> 
                    <button className = "Left-Down" onClick={this.NavDown}> Left-Down </button>
                    <button className = "Down" onClick={this.NavDown}> Down </button>
                    <button className = "Right-Down" onClick={this.NavDown}> Right-Down </button>
                </div>
            </div>

        );
    }
}

export default NavUI;