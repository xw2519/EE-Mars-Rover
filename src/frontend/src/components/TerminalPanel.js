import React from 'react';
import {Helmet} from "react-helmet";
import ws_server from '../components/socketConfig'

import DateTime from './util/time'

class TerminalPanel extends React.Component {
    
    /* Serve landing page */
    render() {
        
        return (
            <>

                <div className="TerminalHeader">
                    System Terminal
                </div>
                
                <div className="TerminalDisplay">
          
                </div>

            </>
        );
    }
}

export default TerminalPanel;
