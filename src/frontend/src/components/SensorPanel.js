import React from 'react';
import {Helmet} from "react-helmet";
import ws_server from '../components/socketConfig'

import DateTime from './util/time'

class SensorReadings extends React.Component {
    
    /* Serve landing page */
    render() {
        
        return (
            <>

            <div className="SensorPanel">            
                <div className = "Time"> 
                    <div className="TimeTitle"> Time: </div>
                    <DateTime></DateTime>
                </div>
                
                <div className = "Battery"> 
                    <div className="BatteryTitle"> Battery Remaining: </div>
                    <div className="BatteryValue"> <output name="BatteryLeft"> </output> % </div>
                </div>

                <div className = "DistanceRemaining"> 
                    <div className="DistanceRemainingTitle"> Distance Remaining: </div>
                    <div className="DistanceRemainingValue"> <output name="DistanceLeft"> </output> (cm) </div>
                </div>

                <div className = "DistanceTravelled"> 
                    <div className="DistanceTravelledTitle"> Distance Travelled: </div>
                    <div className="DistanceTravelledValue"> <output name="DistanceTravelled"> </output> (cm) </div>
                </div>

            </div>
            </>
        );
    }
}

export default SensorReadings;