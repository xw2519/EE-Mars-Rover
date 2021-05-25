import {Helmet} from "react-helmet";
import React from 'react';

import { useCanvas } from './hooks/useCanvas';
import { render } from "@testing-library/react";

function Canvas () {
    
    /* Serve landing page */

    const [ coordinates, setCoordinates, canvasRef, canvasWidth, canvasHeight ] = useCanvas();

    const handleCanvasClick=(event)=>{
        // on each click get current mouse location 
        const currentCoord = { x: event.clientX, y: event.clientY };
        // add the newest mouse location to an array in state 
        console.log("Register");
        setCoordinates([...coordinates, currentCoord]);
    };

    render() 
    {
        return (
            <>
            
            <div className="Canvas"> 
                <canvas 
                    className="App-canvas"
                    ref={canvasRef}
                    width={canvasWidth}
                    height={canvasHeight}
                    onClick={handleCanvasClick} 
                />        
            </div>
    
            </>
        );
    }
        
}
  
export default Canvas;
  