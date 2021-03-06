// User navigation command interface

import React from 'react';
import ws_server from './util/socketConfig';

import { Icon } from '@blueprintjs/core';

// Add in angle and stuffies 
interface NavUIProps {
    myLogger: any;
    prop_dist_value: number;
    prop_angle_value: number;
}

class NavUI extends React.Component<NavUIProps, {}> {
    NavUp = () => {
        console.log("UP");

        if(this.props.prop_dist_value < 10) {
            ws_server.send("F0000" + this.props.prop_dist_value);
        }
        else {
            ws_server.send("F000" + this.props.prop_dist_value);
        }
        
        this.props.myLogger.log('[Sending to rover]: FORWARD rover by ' + this.props.prop_dist_value + ' cm')
    }

    NavDown = () => {
        console.log("DOWN");

        if(this.props.prop_dist_value < 10) {
            ws_server.send("B0000" + this.props.prop_dist_value);
        }
        else {
            ws_server.send("B000" + this.props.prop_dist_value);
        }
        
        this.props.myLogger.info('[Sending to rover]: REVERSE rover ' + this.props.prop_dist_value + ' cm')
    }

    NavRight = () => {
        console.log("RIGHT");

        if(this.props.prop_angle_value < 10) {
            ws_server.send("R0" + this.props.prop_angle_value + "00");
        }
        else {
            ws_server.send("R" + this.props.prop_angle_value + "00");
        }
        
        this.props.myLogger.info('[Sending to rover]: Turn RIGHT by ' + this.props.prop_angle_value)
    }

    NavLeft = () => {
        console.log("LEFT");

        if(this.props.prop_angle_value < 10) {
            ws_server.send("L0" + this.props.prop_angle_value + "00");
        }
        else {
            ws_server.send("L" + this.props.prop_angle_value + "00");
        }

        this.props.myLogger.info('[Sending to rover]: Turn LEFT by ' + this.props.prop_angle_value)
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