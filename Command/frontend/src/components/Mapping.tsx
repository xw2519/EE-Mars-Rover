import { Graph } from '@visx/network';
import {useState} from 'react';
import ws_server from './util/socketConfig'

export type NetworkProps = {
  width: number;
  height: number;
};

interface CustomNode {
  x: number;
  y: number;
  color?: string;
  custom?: string;
  angle?: number
}

interface CustomLink {
  source: CustomNode;
  target: CustomNode;
  dashed?: boolean;
}

export const background = '#1A1A1D';

function LinkNodes(nodes: CustomNode[]) {

  var node_link: CustomLink = {source: {x: 0, y: 0}, target: {x: 0, y: 0}}
  var links: CustomLink[] = [] 

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

  return links
}

export default function Map({ width, height}: NetworkProps) {

  function RoverNode ( angle ) {

    // Plotting rover node icon
    var rotation = 'rotate( ' + angle.angle + ' 14 14 )'

    return (
      // Passes "angle" variable to determine the direction that the rover is facing
      <svg width="40" height="40" fill="currentColor" className="bi-arrow-up-circle-fill" viewBox="0 0 24 24" transform={rotation}>
        <path d="M16 8A8 8 0 1 0 0 8a8 8 0 0 0 16 0zm-7.5 3.5a.5.5 0 0 1-1 0V5.707L5.354 7.854a.5.5 0 1 1-.708-.708l3-3a.5.5 0 0 1 .708 0l3 3a.5.5 0 0 1-.708.708L8.5 5.707V11.5z"/>
      </svg>
    )

  }

  function LocationNode () {

    // Empty node to enable rover pathway plotting
    return (
      <g transform={`translate(${-40 / 2}, ${-40 / 2})`}>
        <svg className="icon-tabler-map-pin" width="30" height="30" viewBox="0 0 30 30" stroke-width="2" stroke="currentColor" fill="none" stroke-linecap="round" stroke-linejoin="round">
        </svg>
      </g>
    )

  }

  function ObstacleNode ( color ) {

    // Plot obstacle nodes 
    return (
      <g transform={`translate(${-40 / 2},${-40 / 2})`}>
        <svg className="icon-tabler-alert-octagon" width="30" height="30" viewBox="0 0 24 24" stroke-width="2" stroke="currentColor" fill={color} stroke-linecap="round" stroke-linejoin="round">
        <path stroke="none" d="M0 0h24v24H0z" fill="none"></path>
        <path 
          d="M8.7 3h6.6c.3 0 .5 .1 .7 .3l4.7 4.7c.2 .2 .3 .4 .3 .7v6.6c0 .3 -.1 .5 -.3 .7l-4.7 4.7c-.2 .2 -.4 .3 -.7 .3h-6.6c-.3 0 -.5 -.1 -.7 -.3l-4.7 -4.7c-.2 -.2 -.3 -.4 -.3 -.7v-6.6c0 -.3 .1 -.5 .3 -.7l4.7 -4.7c.2 -.2 .4 -.3 .7 -.3z">
        </path>
        <line x1="12" y1="8" x2="12" y2="12"></line>
        <line x1="12" y1="16" x2="12.01" y2="16"></line>
        </svg>
      </g>
    )

  }

  const [nodes, set_nodes] = useState<CustomNode[]>([{x:0, y:0, custom:'Rover'}]);

  /*
  function UpdateNodes() {

    // Add current rover location
    var node: CustomNode = {x: (20+nodes[(nodes.length - 1)].x), y: (-10+nodes[(nodes.length - 1)].y), custom: 'Rover'}

    // Change icon on previous rover location 
    for(let i = 0; i < (nodes.length); i++) {
      if(nodes[i].custom === 'Rover') {
        nodes[i].custom = 'Location'
      }
    }

    // Update state
    set_nodes(nodes => [...nodes, node])
  }

  */

  function UpdateNodes(node: CustomNode) {

    // Add current rover location
    var node: CustomNode = {x: (20+nodes[(nodes.length - 1)].x), y: (-10+nodes[(nodes.length - 1)].y), custom: 'Rover'}

    // Change icon on previous rover location 
    for(let i = 0; i < (nodes.length); i++) {
      if(nodes[i].custom === 'Rover') {
        nodes[i].custom = 'Location'
      }
    }

    // Update state
    set_nodes(nodes => [...nodes, node])
  }

  ws_server.onmessage = (e) => {  
    
    // Check if server JSON is intended for map
    var server_message = JSON.parse(e.data); 

    if(server_message.type === "Map") {
      // Sort between Rover location or Obstacle 
      if(server_message.map_type === "Obstacle") {
        var node: CustomNode = {x: (server_message.x_distance+nodes[(nodes.length - 1)].x), y: (server_message.y_distance+nodes[(nodes.length - 1)].y), custom: 'Obstacle', color: server_message.color}

        UpdateNodes(node)
      }
      else {
        // Assign into CustomNode format and pass into "UpdateNodes function"
        var node: CustomNode = {x: (server_message.x_distance+nodes[(nodes.length - 1)].x), y: (server_message.y_distance+nodes[(nodes.length - 1)].y), custom: 'Rover', angle: server_message.angle}

        UpdateNodes(node)
      }
      
    }
  }

  var links: CustomLink[] = [];

  links = LinkNodes(nodes)

  var graph = {
    nodes,
    links,
  };

  // Return SVG map
  return width < 10 ? null : (

    <svg width={width} height={height}>
      <rect width={width} height={height} rx={14} fill={background} />

      <Graph<CustomLink, CustomNode>
        graph={graph}
        top={350}
        left={425}

        nodeComponent={({ node: { custom, color, angle }  }) =>
          // RoverNode takes "angle" parameter to define direction of rover
          // ObstacleNode takes "color" parameter to define the color of the obstacle 
          (custom === 'Rover') ? <RoverNode angle={180}/> : ((custom === 'Location') ? <LocationNode /> : <ObstacleNode color={color} />)
        }

        linkComponent={({ link: { source, target } }) => (
          
          <line
            x1={source.x}
            y1={source.y}
            x2={target.x}
            y2={target.y}
            strokeWidth={2.5}
            stroke="#999"
            strokeOpacity={0.6}
          />

        )}
      />
    </svg>
    
  );
}