import React from 'react';
import ws_server from './util/socketConfig'

import DateTime from './util/time'

class SensorReadings extends React.Component {
    
    /* Serve landing page */
    render() {
        
        ws_server.onmessage = (e) => {  
            //Sensor panel readings
            var sensor_values = JSON.parse(e.data); 
            document.getElementById("BatteryLeft")!.innerHTML = sensor_values.battery_remain;
            document.getElementById("DistanceLeft")!.innerHTML = sensor_values.dist_remain;
            document.getElementById("DistanceTravelled")!.innerHTML = sensor_values.dist_travel;
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