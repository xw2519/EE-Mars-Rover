import React from 'react';
import {Helmet} from "react-helmet";
import ws_server from '../components/socketConfig'

import DateTime from './util/time'

class SensorReadings extends React.Component {
    
    /* Serve landing page */
    render() {
        
        ws_server.onmessage = (e) => {  
            //Sensor panel readings
            document.getElementById("BatteryLeft").innerHTML = parseInt(e.data);
            document.getElementById("DistanceLeft").innerHTML = parseInt(e.data);
            document.getElementById("DistanceTravelled").innerHTML = parseInt(e.data);

            console.log(e.data)
        }

        return (
            <>

            <div className="SensorPanel">            
                <div className = "Time"> 
                    <div className="TimeTitle"> Time: </div>
                    <DateTime></DateTime>
                </div>
                
                <div className = "Battery"> 
                    <div className="BatteryTitle"> Battery Remaining: </div>
                    <div className="BatteryValue"> <output name="BatteryLeft" id="BatteryLeft"> </output> % </div>
                </div>

                <div className = "DistanceRemaining"> 
                    <div className="DistanceRemainingTitle"> Distance Remaining: </div>
                    <div className="DistanceRemainingValue"> <output name="DistanceLeft" id="DistanceLeft"> </output> [cm] </div>
                </div>

                <div className = "DistanceTravelled"> 
                    <div className="DistanceTravelledTitle"> Distance Travelled: </div>
                    <div className="DistanceTravelledValue"> <output name="DistanceTravelled" id="DistanceTravelled"> </output> [cm] </div>
                </div>
            </div>

            </>
        );
    }
}

export default SensorReadings;