import { Graph } from '@visx/network';
import React, {useState} from 'react';

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

    if(i !== nodes.length) {
      node_link = {source: nodes[i], target: nodes[(i+1)]}
    }
    else {
      
    } 
    links.push(node_link)
  }

  return links
}

const ObstacleNode = ({ size = 40 }: { size?: number }) => (
  <g transform={`translate(${-size / 2},${-size / 2})`}>
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

const LocationNode = ({ size = 40 }: { size?: number }) => (
  <g transform={`translate(${-size / 2},${-size / 2})`}>
    <svg xmlns="http://www.w3.org/2000/svg" className="icon icon-tabler icon-tabler-map-pin" width="30" height="30" viewBox="0 0 30 30" stroke-width="2" stroke="currentColor" fill="none" stroke-linecap="round" stroke-linejoin="round">
    <path stroke="none" d="M0 0h24v24H0z" fill="none"></path>
    <circle cx="12" cy="11" r="3"></circle>
    <path d="M17.657 16.657l-4.243 4.243a2 2 0 0 1 -2.827 0l-4.244 -4.243a8 8 0 1 1 11.314 0z"></path>
    </svg>
  </g>
);

const RoverNode = ({ size = 40 }: { size?: number }) => (
  <g transform={`translate(${-size / 2},${-size / 2})`}>
    <svg xmlns="http://www.w3.org/2000/svg" className="icon icon-tabler icon-tabler-current-location" width="40" height="30" viewBox="0 0 22 22" stroke-width="2" stroke="currentColor" fill="none" stroke-linecap="round" stroke-linejoin="round">
    <path stroke="none" d="M0 0h24v24H0z" fill="none"></path>
    <circle cx="12" cy="12" r="3"></circle>
    <circle cx="12" cy="12" r="8"></circle>
    <line x1="12" y1="2" x2="12" y2="4"></line>
    <line x1="12" y1="20" x2="12" y2="22"></line>
    <line x1="20" y1="12" x2="22" y2="12"></line>
    <line x1="2" y1="12" x2="4" y2="12"></line>
    </svg>
  </g>
);

export default function Example({ width, height}: NetworkProps) {

  const [nodes, set_nodes] = useState<CustomNode[]>([{x:0, y:0, custom:'Rover'}]);

  function UpdateNodes() {
    var node: CustomNode = {x: 50, y: 0, custom: 'Obstacle'}
    
    set_nodes(nodes => [...nodes, node])
  }

  var links: CustomLink[] = [
    
  ];

  links = LinkNodes(nodes)

  var graph = {
    nodes,
    links,
  };

  console.log(links)

  return width < 10 ? null : (
    <div>

    <button  className = "Up" onClick={UpdateNodes}> Press </button>

    <svg width={width} height={height}>
      <rect width={width} height={height} rx={14} fill={background} />

      <Graph<CustomLink, CustomNode>
        graph={graph}
        top={350}
        left={425}

        nodeComponent={({ node: { custom, color }  }) =>
          (custom === 'Rover') ? <RoverNode /> : ((custom === 'Location') ? <LocationNode /> : <ObstacleNode/>)
        }

        linkComponent={({ link: { source, target, dashed } }) => (
          
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