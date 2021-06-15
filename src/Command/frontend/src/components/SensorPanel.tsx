// Sensor panel component displaying sensor readings 

import React from 'react';
import DateTime from './util/time'

interface SensorReadingProps {
    distance_travelled: string
    rover_tilt: string
}

function update(ID, value) {
    var element = document.getElementById(ID);
    if(typeof element !== 'undefined' && element !== null) {
        element.innerHTML = value;
    }
}

class SensorReadings extends React.Component<SensorReadingProps, {}> {
    
    /* Serve landing page */
    render() {

        console.log("Distance " + this.props.distance_travelled)

        // Update distance travelled 
        update("DistanceTravelled", this.props.distance_travelled)

        // Update rover tilt
        update("TiltValue", this.props.rover_tilt)

        return (
            <div className="SensorPanel">            
                <div className = "Time"> 
                    <div className="TimeTitle"> Time: </div>
                    <div className="TimeValue"> <DateTime></DateTime> </div>
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

                <div className = "Tilt"> 
                    <div className="TiltTitle"> Rover Tilt: </div>
                    <div className="TiltValue"> <output name="TiltValue" id="TiltValue"> </output> degrees </div>
                </div>
            </div>
        );
    }
}

export default SensorReadings;