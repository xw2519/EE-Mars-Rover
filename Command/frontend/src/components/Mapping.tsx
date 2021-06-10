import { Graph } from '@visx/network';
import {useState} from 'react';
import ws_server from './util/socketConfig'

export type NetworkProps = {
  width: number;
  height: number;
  nodes: CustomNode[];
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

export default function Map({ width, height, nodes}: NetworkProps) {

  function RoverNode ( angle ) {

    // Plotting rover node icon
    var rotation = 'rotate( ' + angle.angle + ' 14 14 )'

    return (
 
      <g transform={`translate(${-40 / 2}, ${-40 / 2})`}>

        <svg width="38" height="38" fill="currentColor" className="bi-arrow-up-circle-fill" viewBox="0 0 24 24" transform={rotation}>
          <path d="M16 8A8 8 0 1 0 0 8a8 8 0 0 0 16 0zm-7.5 3.5a.5.5 0 0 1-1 0V5.707L5.354 7.854a.5.5 0 1 1-.708-.708l3-3a.5.5 0 0 1 .708 0l3 3a.5.5 0 0 1-.708.708L8.5 5.707V11.5z"/>
        </svg>
      </g>

    )

  }

  function LocationNode () {

    // Empty node to enable rover pathway plotting
    return (

        <svg className="icon-tabler-map-pin" width="30" height="30" viewBox="0 0 24 24" stroke-width="2" stroke="currentColor" fill="none" stroke-linecap="round" stroke-linejoin="round">
        </svg>

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

  var links: CustomLink[] = [];

  links = LinkNodes(nodes)

  var graph = {
    nodes,
    links,
  };

  // Return SVG map
  return width < 10 ? null : (

    <svg width={width} height={height}>
      <rect width={width} height={height} rx={2} fill={background} />

      <Graph<CustomLink, CustomNode>
        graph={graph}
        top={350}
        left={425}

        nodeComponent={({ node: { custom, color, angle }  }) =>
          // RoverNode takes "angle" parameter to define direction of rover
          // ObstacleNode takes "color" parameter to define the color of the obstacle 
          (custom === 'Rover') ? <RoverNode angle={angle}/> : ((custom === 'Location') ? <LocationNode /> : <ObstacleNode color={color} />)
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