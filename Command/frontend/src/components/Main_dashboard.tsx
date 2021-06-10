import * as react_console_logger from 'react-console-logger';
import ParentSize from '@visx/responsive/lib/components/ParentSize';
import {Helmet} from "react-helmet";
import { useState, useEffect } from "react";
import ws_server from './util/socketConfig';

import NavUI from './NavUI';
import SensorReadings from './SensorPanel';
import RoverSettings from './RoverSettings';
import NavSettings from './NavSettings'
import Map from './Mapping';

import './Main_dashboard.css';
import './NavUI.css';
import './RoverSettings.css';
import './SensorPanel.css';
import './Mapping.css';
import './NavSettings.css';

const myLogger = new react_console_logger.Logger();

var tracker: number = 1

interface CustomNode {
    x: number;
    y: number;
    custom?: string;
    color?: string;
    angle?: number
}

interface CustomLink {
    source: CustomNode;
    target: CustomNode;
    dashed?: boolean;
}

function LinkNodes(nodes: CustomNode[]) {

    var node_link: CustomLink = {source: {x: 0, y: 0}, target: {x: 0, y: 0}}
    var links: CustomLink[] = [] 
  
    console.log(nodes)
  
    if(nodes.length !== 1) {
      console.log("triggered")
      for(let i = 0; i < (nodes.length-1); i++) {
  
        // Link all nodes
        if(i !== (nodes.length-1)) {
    
          // Do not link obstacles
          if(nodes[i+1].custom === "Obstacle") {
            node_link = {source: nodes[i], target: nodes[(i)]}
          }
          else {
            node_link = {source: nodes[i], target: nodes[(i+1)]}
          }      
        }
    
        links.push(node_link)
      }
    
    }
  
    return links
}

const Dashboard = () => {

    // Variables
    const [dist_value, setDist] = useState(10);
    const [prop_dist_value, setPropDist] = useState(10);
    const [map_type, set_map_type] = useState('');
    var [x_coordinate, set_x_coordinate] = useState(0);
    var [y_coordinate, set_y_coordinate] = useState(0);
    const [x_relative, set_x_relative] = useState<number>(0);
    const [y_relative, set_y_relative] = useState<number>(0);
    const [angle, set_angle] = useState(0);
    const [color, set_color] = useState('');
    const [battery_left, set_battery_left] = useState(0);
    const [distance_travelled, set_distance_travelled] = useState(0);
    const [distance_left, set_distance_left] = useState(0);

    var [nodes, set_nodes] = useState<CustomNode[]>([{x:0, y:0, custom:'Rover', angle:0}]);

    useEffect(() => {
        const timeOutId = setTimeout(() => setPropDist(dist_value), 500);
        return () => clearTimeout(timeOutId);
    }, [dist_value]);

    function UpdateNodes() {

        if(tracker === 1) {
            if(map_type === "Rover") {
                var x = parseInt(x_coordinate.toString()) + parseInt(x_relative.toString()) 
                var y = parseInt(y_coordinate.toString()) + parseInt(y_relative.toString())

                var node: CustomNode = {x: (x), y: (y), custom: 'Rover', angle: angle}

                set_x_relative(x)
                set_y_relative(y)

                set_nodes(
                    nodes = [...nodes, node]
                );

                var links: CustomLink[] = [];

                links = LinkNodes(nodes)

            }
            else if(map_type === "Obstacle") {
                var node: CustomNode = {x: ((x_coordinate+x_relative)*0.4), y: ((y_coordinate+y_relative)*0.4), custom: 'Obstacle', color: color}
            }
            
            // Change icon on previous rover location 
            for(let i = 0; i < (nodes.length); i++) {
                if(nodes[i].custom === 'Rover') {
                    nodes[i].custom = 'Location'
                }
            }
        
            // Update state
            set_nodes(nodes => [...nodes, node])
        
            tracker = 2
        }
        else {
            if(map_type === "Rover") {
                var x = parseInt(x_coordinate.toString()) + parseInt(x_relative.toString()) 
                var y = parseInt(y_coordinate.toString()) + parseInt(y_relative.toString())

                var node: CustomNode = {x: (x), y: (y), custom: 'Rover', angle: angle}

                set_x_relative(x)
                set_y_relative(y)

            }
            else if(map_type === "Obstacle") {
                var node: CustomNode = {x: ((x_coordinate+x_relative)*0.4), y: ((y_coordinate+y_relative)*0.4), custom: 'Obstacle', color: color}
            }
            
            // Change icon on previous rover location 
            for(let i = 0; i < (nodes.length); i++) {
                if(nodes[i].custom === 'Rover') {
                    nodes[i].custom = 'Location'
                }
            }
        
            // Update state
            set_nodes(nodes => [...nodes, node])
        }
    }

    ws_server.onmessage = (e) => {  
            
        console.log(e.data)
    
        // Check if server JSON is intended for terminal
        var server_message = JSON.parse(e.data); 
    
        console.log(server_message)
    
        if(server_message.type === "Terminal") {
            myLogger.info(server_message.message)
        }

        else if(server_message.type === "Map") {

            set_x_coordinate(
                x_coordinate = server_message.x_distance
            );

            set_y_coordinate(
                y_coordinate = server_message.y_distance
            );

            if(server_message.map_type == "Rover") {
                set_map_type("Rover")
                set_angle(server_message.angle)

                UpdateNodes()

            }
            else {
                console.log("Obstacle")
                set_map_type("Obstacle")
                set_color(server_message.color)

                UpdateNodes()
            }

        }

        else if(server_message.type === "Energy") {
            set_battery_left(server_message.battery_left)
            set_distance_travelled(server_message.distance_travelled)
            set_distance_left(server_message.distance_left)
        }
    
    }

    /* Serve landing page */
    return (
        <>
            <Helmet>
                <title> Mars Rover Command Center </title>
            </Helmet>

            <div className="App">

                <div className="container">
                    
                    <div className="HeaderTitle">
                        <h1> Mars Rover Command Center </h1>

                    </div>
                    
                    <div className="grid SensorReading"> Sensor Panel <SensorReadings/> </div>
        
                    <div className="grid Map"> 
                        <ParentSize>{({ width, height }) => <Map width={width} height={height} nodes={nodes}/>}</ParentSize>
                    
                    </div>

                    <div className="grid Terminal"> 
                        <div className="TerminalHeader">
                            System Terminal
                        </div>
                        <div className="TerminalLog">
                        <react_console_logger.ConsoleLogger
                                logger={myLogger}
                                style={{
                                    position: 'absolute',
                                    overflowY: 'scroll',
                                    width: '95%',
                                    height: '100%',
                                    lineHeight: 3,
                                    background: '#272727',
                                    infoColor: '#D1E8E2',
                                    textAlign: 'left'
                                }}
                            />
                        </div>

                    </div>
        
                    <div className="grid RoverSettings"> Rover Quick Settings <RoverSettings myLogger={myLogger}/> </div>
        
                    <div className="grid NavigationUI"> Navigation Panel <NavUI myLogger={myLogger} prop_dist_value={prop_dist_value}/> </div>
                    
                    <div className="grid CommandSettings"> Navigation Settings <NavSettings myLogger={myLogger} setDist={setDist}/> </div>
                </div>

            </div>
        </>
    );
};
  
export default Dashboard;
  