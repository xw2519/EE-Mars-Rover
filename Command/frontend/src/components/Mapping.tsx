// Map component of the Main dashboard

import { Graph } from '@visx/network';

export type NetworkProps = {
  width: number;
  height: number;
  nodes: CustomNode[];
  links: CustomLink[];
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

export default function Map({ width, height, nodes, links}: NetworkProps) {

  function RoverNode ( angle ) {

    console.log(angle.angle)

    // Plotting rover node icon
    var rotation = 'rotate( ' + angle.angle + ' 14 14 )'

    return (
 
      <g transform={`translate(${-30 / 2}, ${-33 / 2})`}>

        <svg width="55" height="50" fill="currentColor" className="bi-arrow-up-circle-fill" viewBox="0 0 30 30" transform={rotation}>
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

    var fill:string = '';
    
    if(color.color === "R") { fill = "#FF0000" }
    if(color.color === "G") { fill = "#008000" }
    if(color.color === "B") { fill = "#0000FF" }
    if(color.color === "Y") { fill = "#FFFF00" }
    if(color.color === "P") { fill = "#FFB6C1" }

    // Plot obstacle nodes 
    return (
      <g transform={`translate(${-40 / 2},${-40 / 2})`}>
        <svg className="icon-tabler-alert-octagon" width="30" height="30" viewBox="0 0 35 35" stroke-width="2" stroke={fill} fill="#1A1A1D" stroke-linecap="round" stroke-linejoin="round">
        <path stroke="none" d="M0 0h24v24H0z" fill={color}></path>
        <path 
          d="M8.7 3h6.6c.3 0 .5 .1 .7 .3l4.7 4.7c.2 .2 .3 .4 .3 .7v6.6c0 .3 -.1 .5 -.3 .7l-4.7 4.7c-.2 .2 -.4 .3 -.7 .3h-6.6c-.3 0 -.5 -.1 -.7 -.3l-4.7 -4.7c-.2 -.2 -.3 -.4 -.3 -.7v-6.6c0 -.3 .1 -.5 .3 -.7l4.7 -4.7c.2 -.2 .4 -.3 .7 -.3z">
        </path>
        <line x1="12" y1="8" x2="12" y2="12"></line>
        <line x1="12" y1="16" x2="12.01" y2="16"></line>
        </svg>
      </g>
    )
    
  }

  console.log(nodes)
  console.log(links)

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
        top={550}
        left={150}

        nodeComponent={({ node: { custom, color, angle }  }) =>
          // RoverNode takes "angle" parameter to define direction of rover
          // ObstacleNode takes "color" parameter to define the color of the obstacle 
          (custom === 'Rover') ? <RoverNode angle={angle}/> : ((custom === 'Location') ? <LocationNode /> : <ObstacleNode color={color} />)
        }
      />
    </svg>
    
  );
}