import react_console_logger from 'react-console-logger';
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

const Dashboard = () => {

    var map_type = '';
    var angle: number = 0;
    var color: string = '';
    var x_rover_current: number = 0;
    var y_rover_current: number = 0;
    var x_plot_previous: number = 0;
    var y_plot_previous: number = 0;
    var x_rover_previous: number = 0;
    var y_rover_previous: number = 0;

    // var nodes: CustomNode[] = [{x:0, y:0, custom:'Rover', angle:0}];

    // Variables
    const [dist_value, setDist] = useState(10);
    const [prop_dist_value, setPropDist] = useState(10);
    
    const [battery_left, set_battery_left] = useState(0);
    const [distance_travelled, set_distance_travelled] = useState(0);
    const [distance_left, set_distance_left] = useState(0);

    const [nodes, set_nodes] = useState<CustomNode[]>([{x:0, y:0, custom:'Rover', angle:0}]);
    const [links, set_links] = useState<CustomLink[]>([{source: {x: 0, y: 0}, target: {x: 0, y: 0}}]);
   
    useEffect(() => {
        const timeOutId = setTimeout(() => setPropDist(dist_value), 500);
        return () => clearTimeout(timeOutId);
    }, [dist_value]);

    ws_server.onmessage = (e) => {  
        // Check if server JSON is intended for terminal
        var server_message = JSON.parse(e.data); 
    
        console.log("Receiving JSON: ")
        console.log(server_message)
    
        if(server_message.type === "Terminal") { myLogger.info(server_message.message) }

        else if(server_message.type === "Map") {

            x_rover_current = server_message.x_distance
            y_rover_current = server_message.y_distance

            if(server_message.map_type === "Rover") {
                map_type = "Rover"
                angle = server_message.angle
                UpdateNodes()
                LinkNodes(nodes)
            }
            else {
                map_type = "Obstacle"
                color = server_message.color
                UpdateNodes()
                LinkNodes(nodes)
            }
        }

        else if(server_message.type === "Energy") {
            set_battery_left(server_message.battery_left)
            set_distance_travelled(server_message.distance_travelled)
            set_distance_left(server_message.distance_left)
        }
    }

    function useForceUpdate(){
        const [value, setValue] = useState(0); // integer state
        return () => setValue(value => value + 1); // update the state to force render
    }

    const forceUpdate = useForceUpdate();
    
    function UpdateNodes() {

        if(map_type === "Rover") {
            // Rover - Map plotting logic
            var x_plot = 0
            var y_plot = 0

            // Multipled by constant to caliberate zoom
            x_rover_current= parseInt(x_rover_current.toString())
            y_rover_current = parseInt(y_rover_current.toString())

            // Determine direction
            if(x_rover_current === x_rover_previous) {
                // x-axis not changing
                x_plot = x_plot_previous

                // y-axis not changing 
                if(y_rover_current === y_rover_previous) { y_plot = y_plot_previous }
                else {
                    // y-axis changing 
                    y_plot = (y_rover_current + y_plot_previous)
                    y_plot_previous = y_plot
                }
            }
            else {
                // x-axis is changing 
                x_plot = (x_rover_current + x_plot_previous)

                // y-axis not changing 
                if(y_rover_current === y_rover_previous) { y_plot = y_plot_previous }
                else {
                    // y-axis changing 
                    y_plot = (y_rover_current + y_plot_previous)
                    y_plot_previous = y_plot
                }
                x_plot_previous = x_plot
            }
            
            // Assign coordinate into node 
            var node: CustomNode = {x: (x_plot*3), y: (y_plot*3), custom: 'Rover', angle: angle}

            // Update distance travelled
            set_distance_travelled((Math.abs(x_plot) + Math.abs(y_plot)))

            // Update node array 
            set_nodes((nodes) => {
                nodes.push(node)
                return nodes;
            });

            // Update icon on previous rover location 
            for(let i = 0; i < (nodes.length-1); i++) { 
                if(nodes[i].custom === 'Rover') { 
                    nodes[i].custom = 'Location' 
                } 
            }
        }
        else {
            // Obstacle - Map plotting logic
            var x_plot = 0
            var y_plot = 0

            var x_obstacle_current = 0
            var y_obstacle_current = 0

            x_obstacle_current = parseInt(x_rover_current.toString())*3
            y_obstacle_current = parseInt(y_rover_current.toString())*3

            // Assign coordinates 
            x_plot = x_obstacle_current + x_plot_previous
            y_plot = y_obstacle_current + y_plot_previous
            
            // Assign coordinate into node 
            var node: CustomNode = {x: (x_plot), y: (y_plot), custom: 'Obstacle', color: color}

            // Update node array 
            set_nodes((nodes) => {
                nodes.push(node)
                return nodes;
            });
        }
        
    }
    
    function LinkNodes(nodes: CustomNode[]) {

        var node_link: CustomLink = {source: {x: 0, y: 0}, target: {x: 0, y: 0}}
    
        for(let i = 0; i < (nodes.length-1); i++) {
    
            // Link all nodes
            if(i !== (nodes.length)) {
                
                // Do not link obstacles
                console.log(nodes[i+1].custom)
                if(nodes[i+1].custom === "Obstacle") {
                    node_link = {source: nodes[i], target: nodes[(i)]}
                }
                else { 
                    node_link = {source: nodes[i], target: nodes[(i+1)]}
                }      
            }
            set_links((links) => {
                links.push(node_link)
                return links;
            });
        } 

        forceUpdate()
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
                    
                    <div className="grid SensorReading"> Sensor Panel <SensorReadings distance_travelled={distance_travelled}/> </div>
        
                    <div className="grid Map"> 
                        <ParentSize>{({ width, height }) => <Map width={width} height={height} nodes={nodes} links={links}/>}</ParentSize>
                    
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
  