import {Helmet} from "react-helmet";
import React from 'react';

import NavUI from './NavUI';
import SensorReadings from './SensorPanel';

import './dashboard.css';
import './NavUI.css';
import './SensorPanel.css';

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
                    </div>
        
                    <div className="grid SensorReading"> Sensor Panel <SensorReadings/> </div>
        
                    <div className="grid Map"> Map </div>

                    <div className="grid Terminal"> Terminal </div>
        
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
  