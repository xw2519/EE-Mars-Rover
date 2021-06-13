// Rover settings interface

import { useState } from "react";
import Switch from "react-switch";
import ws_server from './util/socketConfig';

const RoverSettings = (NavUIProps) => {
    const [checked, setChecked] = useState(false);

    const handleChange = nextChecked => {
      setChecked(nextChecked);

      console.log(!checked)

      if(!checked) {
        NavUIProps.myLogger.log('[Sending to rover]: Autonomous mode activated')
        ws_server.send("Aon");
      }
      else {
        NavUIProps.myLogger.log('[Sending to rover]: Autonomous mode deactivated')
        ws_server.send("Aoff");
      }

    };

    return (
    <div className="Rover_Settings"> 
      <div className="Auto_mode">

        <div className="Auto_mode_title"> 
            <span>Autonomous mode </span>
        </div>

        <div className="Auto_mode_switch"> 
            <label>
                <Switch
                    className="react-switch"
                    onChange={handleChange}
                    checked={checked}
                />
            </label>
        </div>
            
      </div>

    </div>
    );
};

export default RoverSettings