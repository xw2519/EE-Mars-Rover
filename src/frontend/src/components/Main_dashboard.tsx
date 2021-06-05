import React, {useState} from 'react';

import * as react_console_logger from 'react-console-logger';
import ParentSize from '@visx/responsive/lib/components/ParentSize';
import {Helmet} from "react-helmet";

import NavUI from './NavUI';
import SensorReadings from './SensorPanel';
import RoverSettings from './RoverSettings';
import Example from './Mapping';

import './Main_dashboard.css';
import './NavUI.css';
import './RoverSettings.css';
import './SensorPanel.css';
import './Mapping.css';

const myLogger = new react_console_logger.Logger();

const Dashboard = () => {

    /* Serve landing page */

    return (
        <>
        <Helmet>
            <title> Mars Rover Command Center </title>
        </Helmet>

        <div className="App">

            <div className="container">
                
                <div className="HeaderTitle">
                    <h1> Mars Rover Command Center </h1>

                </div>
                
                <div className="grid SensorReading"> Sensor Panel <SensorReadings/> </div>
    
                <div className="grid Map"> 
                    <ParentSize>{({ width, height }) => <Example width={width} height={height} />}</ParentSize>
                
                </div>

                <div className="grid Terminal"> 
                    <div className="TerminalHeader">
                        System Terminal
                    </div>
                    <div className="TerminalLog">
                    <react_console_logger.ConsoleLogger
                            logger={myLogger}
                            style={{
                                position: 'absolute',
                                overflowY: 'scroll',
                                width: '95%',
                                height: '100%',
                                lineHeight: 3,
                                background: '#272727',
                                infoColor: '#D1E8E2',
                                textAlign: 'left'
                            }}
                        />
                    </div>

                </div>
    
                <div className="grid RoverSettings"> Rover Quick Settings <RoverSettings myLogger={myLogger}/> </div>
    
                <div className="grid NavigationUI"> Navigation Panel <NavUI myLogger={myLogger}/> </div>
                
                <div className="grid CommandSettings"> UI Settings </div>
            </div>

        </div>

        </>
    );
};
  
export default Dashboard;
  