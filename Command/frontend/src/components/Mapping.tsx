import { Graph } from '@visx/network';
import React, {useState} from 'react';
import ws_server from './util/socketConfig'
import { Tooltip } from "react-svg-tooltip";

export type NetworkProps = {
  width: number;
  height: number;
};

interface CustomNode {
  x: number;
  y: number;
  color?: string;
  custom?: string;
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

  const circleRef = React.createRef<SVGCircleElement>();

  const RoverNode = ( ) => (
    <g transform={`translate(${-40 / 2},${-40 / 2})`}>
      <svg xmlns="http://www.w3.org/2000/svg" className="icon icon-tabler icon-tabler-current-location" width="40" height="30" viewBox="0 0 40 30" stroke-width="2" stroke="currentColor" fill="none" stroke-linecap="round" stroke-linejoin="round" onClick={UpdateNodes}>
        <path stroke="none" d="M0 0h24v24H0z" fill="none"></path>
        <circle cx="12" cy="12" r="2" fill="currentColor"ref={circleRef}></circle>
        <circle cx="12" cy="12" r="8" ></circle>
        <line x1="12" y1="2" x2="12" y2="4"></line>
        <line x1="12" y1="20" x2="12" y2="22"></line>
        <line x1="20" y1="12" x2="22" y2="12"></line>
        <line x1="2" y1="12" x2="4" y2="12"></line>

      </svg>
    </g>
    
  );

  const LocationNode = ({ size = 40 }: { size?: number }) => (
    <g transform={`translate(${-size / 2},${-size / 2})`}>
      <svg xmlns="http://www.w3.org/2000/svg" className="icon icon-tabler icon-tabler-map-pin" width="30" height="30" viewBox="0 0 30 30" stroke-width="2" stroke="currentColor" fill="none" stroke-linecap="round" stroke-linejoin="round">
      
      </svg>
    </g>
  );

  const ObstacleNode = ( color ) => (
    <g transform={`translate(${-40 / 2},${-40 / 2})`}>
      <svg  xmlns="http://www.w3.org/2000/svg" className="icon icon-tabler icon-tabler-alert-octagon" width="30" height="30" viewBox="0 0 24 24" stroke-width="2" stroke="currentColor" fill="none" stroke-linecap="round" stroke-linejoin="round">
      <path stroke="none" d="M0 0h24v24H0z" fill="none"></path>
      <path 
        d="M8.7 3h6.6c.3 0 .5 .1 .7 .3l4.7 4.7c.2 .2 .3 .4 .3 .7v6.6c0 .3 -.1 .5 -.3 .7l-4.7 4.7c-.2 .2 -.4 .3 -.7 .3h-6.6c-.3 0 -.5 -.1 -.7 -.3l-4.7 -4.7c-.2 -.2 -.3 -.4 -.3 -.7v-6.6c0 -.3 .1 -.5 .3 -.7l4.7 -4.7c.2 -.2 .4 -.3 .7 -.3z">
      </path>
      <line x1="12" y1="8" x2="12" y2="12"></line>
      <line x1="12" y1="16" x2="12.01" y2="16"></line>
      </svg>
    </g>
  );

  const [nodes, set_nodes] = useState<CustomNode[]>([{x:0, y:0, custom:'Rover'}]);

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

  ws_server.onmessage = (e) => {  
    
    // Check if server JSON is intended for map
    var server_message = JSON.parse(e.data); 

    if(server_message.type === "Map") {
      // Assign into CustomNode format and pass into "UpdateNodes function"

    }

  }

  var links: CustomLink[] = [
    
  ];

  links = LinkNodes(nodes)

  var graph = {
    nodes,
    links,
  };

  // Return SVG map
  return width < 10 ? null : (
    <div>

    <svg width={width} height={height}>
      <rect width={width} height={height} rx={14} fill={background} />

      <Graph<CustomLink, CustomNode>
        graph={graph}
        top={350}
        left={425}

        nodeComponent={({ node: { custom, color }  }) =>
          (custom === 'Rover') ? <RoverNode /> : ((custom === 'Location') ? <LocationNode /> : <ObstacleNode color={color} />)
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
    </div>
  );
}