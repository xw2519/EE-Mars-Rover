import './App.css';
import {Helmet} from "react-helmet";
import React from 'react';

import {AreaChart, CartesianGrid, Area, XAxis, YAxis} from 'recharts'

class App extends React.Component {
  state = {data: [], count: 0}

  /* Establish websocket connection */
  componentDidMount() {
    const ws = new WebSocket('ws://localhost:9000/ws')
    ws.onmessage = this.onMessage

    this.setState({
      ws: ws,
      // Create an interval to send echo messages to the server
      interval: setInterval(() => ws.send('echo'), 1000)
    })
  }

  componentWillUnmount() {
    const {ws, interval} = this.state;
    ws.close()
    clearInterval(interval)
  }

  onMessage = (ev) => {
    const recv = JSON.parse(ev.data)
    const {data, count} = this.state
    let newData = [...data]
    // Remove first data if we received more than 20 values
    if (count > 20) {
      newData = newData.slice(1)
    }
    newData.push({value: recv.value, index: count})
    this.setState({data: newData, count: count + 1})
  }

  /* Serve landing page */
  render() {
    return (
      <>

      <Helmet>
            <title> Mars Rover Command Center </title>
      </Helmet>
      <div className="App">
      <div class="header">
        <a href="#default" class="logo">Logo</a>
        <h1> Mars Rover Command Center </h1>
      <div class="MainHeaderTitle">
  
        </div>
      </div> 
      
      <div class="container">
          <div class="grid SensorReading">
            <div class = "PanelHeading"> Sensor reading </div> 

            <div class = "Time"> 
              Time: <output name="Time"></output>
            </div>
          
            <div class = "BatteryReading"> 
              Battery Remaining: <output name="BatteryLeft"></output>
            </div>

            <div class = "DistanceRemaining"> 
              Distance Remaining: <output name="DistanceLeft"></output>
            </div>

            <div class = "DistanceTravelled"> 
              Distance Travelled: <output name="DistanceTravelled"></output>
            </div>
          </div>

          <div class="grid Map"> Map </div>
          <div class="grid Terminal"> Terminal </div>

          <div class="grid RoverSettings"> 
            Rover Quick Settings 

          </div>

          <div class="grid NavigationUI">
            Nav UI

            <div class="NavButtons">
              <div class="Top"> 
                <button class = "Up"> 
                  Up
                </button>
              </div>
              <div class="Middle"> 
                <button class = "Left"> 
                  Left
                </button>
                <button class = "Halt"> 
                  Halt
                </button>
                <button class = "Right"> 
                  Right
                </button>
              </div>
              <div class="Bottom"> 
                <button class = "Down"> 
                  Down
                </button>
              </div>
            </div>

          </div>
          <div class="grid CommandSettings"> UI Settings </div>
      </div>
      </div>

      </>
    );
  }
}
export default App;
