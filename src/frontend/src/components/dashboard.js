import {Helmet} from "react-helmet";
import React from 'react';

import {Logger, ConsoleLogger} from 'react-console-logger';

import NavUI from './NavUI';
import SensorReadings from './SensorPanel';
import Canvas from './canvas';

import './dashboard.css';
import './NavUI.css';
import './SensorPanel.css';
import './canvas.css'

const myLogger = new Logger();

class Dashboard extends React.Component {
   
    /* Serve landing page */
    render() {
        
        return (
            <>
            <Helmet>
                <title> Mars Rover Command Center </title>
                
            </Helmet>

            <div classNameName="App">

                <div className="container">
                    <div className="HeaderTitle">
                        <a href="#default" className="logo">Logo</a>
                        <h1> Mars Rover Command Center </h1>
                        <button
                        onClick={() => myLogger.info('Hi')}
                        style={{
                            width: 100,
                            height: 30
                        }}
                        >
                        click!
                        </button>
                    </div>
        
                    <div className="grid SensorReading"> Sensor Panel <SensorReadings/> </div>
        
                    <div className="grid Map"> <Canvas/> </div>

                    <div className="grid Terminal"> 
                        <div className="TerminalHeader">
                            System Terminal
                        </div>
                        <div className="TerminalLog">
                        <ConsoleLogger
                                logger={myLogger}
                                style={{
                                    position: 'relative',
                                    overflowY: 'scroll',
                                    width: '95%',
                                    lineHeight: 2,
                                    background: 'black',
                                    textAlign: 'left',
                                    color: 'blue'
                                }}
                            />
                        </div>

                    </div>
        
                    <div className="grid RoverSettings"> Rover Quick Settings </div>
        
                    <div className="grid NavigationUI"> Navigation Panel <NavUI/> </div>
                    
                    <div className="grid CommandSettings"> UI Settings </div>
                </div>

            </div>
    
            </>
        );
    }
  }
  export default Dashboard;
  