// User navigation settings interface

import {useState} from 'react';

const NavSettings = (NavSettings) => {

  const [value, setValue] = useState(10);

  const handleDistChange = (event) => {
    // Limiting the range of allowed input
    if(event >= 100) {
      setValue(99)
      NavSettings.setDist(99)
    }
    else {
      NavSettings.setDist(event)
      setValue(event)
    }
    
  };

  return (
  <div className="Rover_Settings"> 
    <div className="Unit">

      <div className="Unit_title"> 
          <span> Unit distance [cm] </span>
      </div>

      <div className="Unit_switch"> 
          <label>
            <input
              id="Dist_value"
              value={value}
              className="Dist_value"
              type = "number"
              placeholder="Unit"
              defaultValue="10"
              size={10}
              max={99}
              min={1}
              maxLength={2}
              onChange={e => handleDistChange(e.target.value)}

            />
          </label>
      </div>
          
    </div>

  </div>
  );
};

export default NavSettings