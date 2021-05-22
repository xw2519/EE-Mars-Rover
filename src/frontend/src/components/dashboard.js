import './dashboard.css';
import './NavUI.css';

import {Helmet} from "react-helmet";
import React from 'react';
import NavUI from './NavUI';

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
        
                    <div className="grid SensorReading">
                        <div className = "PanelHeading"> Sensor reading </div> 
            
                        <div className = "Time"> 
                            Time: <output name="Time"></output>
                        </div>
                        
                        <div className = "BatteryReading"> 
                            Battery Remaining: <output name="BatteryLeft"></output>
                        </div>
            
                        <div className = "DistanceRemaining"> 
                            Distance Remaining: <output name="DistanceLeft"></output>
                        </div>
            
                        <div className = "DistanceTravelled"> 
                            Distance Travelled: <output name="DistanceTravelled"></output>
                        </div>
                    </div>
        
                    <div className="grid Map"> Map </div>

                    <div className="grid Terminal"> Terminal </div>
        
                    <div className="grid RoverSettings"> Rover Quick Settings </div>
        
                    <div className="grid NavigationUI"> 
                        Navigation UI 
                        <NavUI /> 
                    </div>
                    
                    <div className="grid CommandSettings"> UI Settings </div>
                </div>
            </div>
    
            </>
        );
    }
  }
  export default Dashboard;
  