var value

const NavSettings = (NavSettings) => {

  const handleDistChange = (event) => {

    if(event >= 100) {
      value = 99
      NavSettings.setDist(99)
    }
    else {
      NavSettings.setDist(event)
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
                type = "number"
                placeholder="Unit"
                defaultValue="10"
                size={10}
                max={99}
                min={1}
                maxLength={3}
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