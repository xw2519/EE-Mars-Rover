// User navigation settings interface

import {useState} from 'react';

const NavSettings = (NavSettings) => {

  const [value, setValue] = useState(10);

  const [angle, setAngle] = useState(90);

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

  const handleAngleChange = (event) => {
    // Limiting the range of allowed input
    if(event >= 90) {
      setAngle(90)
      NavSettings.setAngle(90)
    }
    else {
      NavSettings.setAngle(event)
      setAngle(event)
    }
  };

  return (
  <div className="Rover_Settings"> 
    <div className="Unit">

      <div className="Unit_title"> 
          <span> Unit Distance [cm] </span>
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

    <div className="Angle">

      <div className="Angle_title"> 
          <span> Turning Angle </span>
      </div>

      <div className="Angle_switch"> 
          <label>
            <input
              id="Angle_value"
              value={angle}
              className="Angle_value"
              type = "number"
              placeholder="Angle"
              defaultValue="90"
              size={10}
              max={90}
              min={1}
              maxLength={2}
              onChange={e => handleAngleChange(e.target.value)}
            />
          </label>
      </div>
          
    </div>

  </div>
  );
};

export default NavSettings