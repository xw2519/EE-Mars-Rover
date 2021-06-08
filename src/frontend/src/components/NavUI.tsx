import React from 'react';
import ws_server from './util/socketConfig';

import { Icon } from '@blueprintjs/core';

// Add in angle and stuffies 
interface NavUIProps {
    myLogger: any;
    prop_dist_value: number
}

class NavUI extends React.Component<NavUIProps, {}> {
    NavUp = () => {
        console.log("UP");
        ws_server.send("F00" + this.props.prop_dist_value);
        this.props.myLogger.log('[Sending to rover]: FORWARD rover by ' + this.props.prop_dist_value + ' cm')
    }

    NavDown = () => {
        console.log("DOWN");
        ws_server.send("B00" + this.props.prop_dist_value);
        this.props.myLogger.info('[Sending to rover]: REVERSE rover ' + this.props.prop_dist_value + ' cm')
    }

    NavRight = () => {
        console.log("RIGHT");
        ws_server.send("R9000");
        this.props.myLogger.info('[Sending to rover]: Turn RIGHT')
    }

    NavLeft = () => {
        console.log("LEFT");
        ws_server.send("L9000");
        this.props.myLogger.info('[Sending to rover]: Turn LEFT')
    }

    NavHalt = () => {
        console.log("HALT");
        ws_server.send("S0000");
        this.props.myLogger.info('[Sending to rover]: STOP rover')
    }

    Start = () => {
        console.log("START");
        ws_server.send("START");
    }
    
    /* Serve landing page */
    render() {
        
        return (
    
            <div className="NavButtons">

                <div className="Top"> 
                    
                    <button  className = "Up" onClick={this.NavUp}> <Icon icon="arrow-up" color="#D1E8E2" />  </button>
           
                </div>
                <div className="Middle"> 
                    <button className = "Left" onClick={this.NavLeft}> <Icon icon="arrow-left" color="#D1E8E2" />  </button>
                    <button className = "Halt" onClick={this.NavHalt}> <Icon icon="ban-circle" color="#D1E8E2" iconSize={18} /> </button>
                    <button className = "Right" onClick={this.NavRight}> <Icon icon="arrow-right" color="#D1E8E2" /></button>
                </div>
                <div className="Bottom"> 

                    <button className = "Down" onClick={this.NavDown}> <Icon icon="arrow-down" color="#D1E8E2" /> </button>
    
                </div>
            </div>

        );
    }
}

export default NavUI;