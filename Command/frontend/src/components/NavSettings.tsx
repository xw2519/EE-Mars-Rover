import { useState, useEffect } from "react";
import Switch from "react-switch";
import ws_server from './util/socketConfig';
import { TextInputField, Button } from 'evergreen-ui'

const NavSettings = (NavSettings) => {

    const handleDistChange = (event) => {

      NavSettings.setDist(event)
      
    };

    return (
    <div className="Rover_Settings"> 
      <div className="Unit">

        <div className="Unit_title"> 
            <span> Unit distance [cm] </span>
        </div>

        <div className="Unit_switch"> 
            <label>
              <TextInputField
                  placeholder="Unit"
                  defaultValue="10"
                  inputHeight={50}
                  inputWidth={70}
                  className="Dist_value"
                  onChange={e => handleDistChange(e.target.value)}
              />
            </label>
        </div>
            
      </div>

    </div>
    );
};

export default NavSettings