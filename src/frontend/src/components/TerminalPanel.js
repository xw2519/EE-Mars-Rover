import React from 'react';
import {Helmet} from "react-helmet";
import ws_server from '../components/socketConfig'

class TerminalPanel extends React.Component {

    TerminalDisplay = () => {
        console.log = function(message) {
            document.getElementById('log').innerHTML = message;
        };
        console.log("test")
        console.log('your result');
    }

    render() {

        return (
            <>

            <div className="TerminalHeader">
                System Terminal
            </div>
            
            {this.TerminalDisplay}
            
            <div id="log">  </div>
            
            </>
        );
    }
}

export default TerminalPanel;
