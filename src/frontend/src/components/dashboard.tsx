import React from 'react';

import * as react_console_logger from 'react-console-logger';

import NavUI from './NavUI';
import SensorReadings from './SensorPanel';

import './dashboard.css';
import './NavUI.css';
import './SensorPanel.css';
import './canvas.css'

const myLogger = new react_console_logger.Logger();

class Dashboard extends React.Component {
  
    /* Serve landing page */
    render() {
    
        return (
            <>
            <title> Mars Rover Command Center </title>

            <div className="App">

                <div className="container">
                    <div className="HeaderTitle">
                        <h1> Mars Rover Command Center </h1>
                    </div>
        
                    <div className="grid SensorReading"> Sensor Panel <SensorReadings/> </div>
        
                    <div className="grid Map"> </div>

                    <div className="grid Terminal"> 
                        <div className="TerminalHeader">
                            System Terminal
                        </div>
                        <div className="TerminalLog">
                        <react_console_logger.ConsoleLogger
                                logger={myLogger}
                                style={{
                                    position: 'relative',
                                    overflowY: 'scroll',
                                    width: '95%',
                                    lineHeight: 2,
                                    background: '#272727',
                                    infoColor: '#D1E8E2',
                                    textAlign: 'left'
                                }}
                            />
                        </div>

                    </div>
        
                    <div className="grid RoverSettings"> Rover Quick Settings </div>
        
                    <div className="grid NavigationUI"> Navigation Panel <NavUI myLogger={myLogger}/> </div>
                    
                    <div className="grid CommandSettings"> UI Settings </div>
                </div>

            </div>
    
            </>
        );
    }
  }
  export default Dashboard;
  