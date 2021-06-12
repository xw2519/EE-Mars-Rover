// Login page component

import { useState } from 'react';
import './Login.css';
import PropTypes from 'prop-types';
import { TextInputField, Button } from 'evergreen-ui'

import {server_IP_address} from './util/socketConfig';

async function loginUser(credentials) {
    return fetch('http://' + server_IP_address + ':8000/login', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      },
      body: JSON.stringify(credentials)
    })
      .then(data => data.json())
}

export default function Login({ setToken }) {

    const [username, setUserName] = useState();
    const [password, setPassword] = useState();

    const handleSubmit = async e => {
        e.preventDefault();
        const token = await loginUser({
          username,
          password
        });
        setToken(token);
    }

    return(
        <div className="login-wrapper">
            <div className="login_title_1"> 
                Mars Rover Command Center 
            </div>

            <div className="login_title_2"> 
                Login 
            </div>

            <div className="login_field"> 
                <form onSubmit={handleSubmit}>

                    <TextInputField
                        placeholder="Username"
                        inputHeight={50}
                        inputWidth={300}
                        onChange={e => setUserName(e.target.value)}
                    />
    
                    <TextInputField
                        placeholder="Password"
                        inputHeight={50}
                        inputWidth={300}
                        onChange={e => setPassword(e.target.value)}
                    />

                    <Button marginTop={60} appearance="primary" size="large">
                        Login
                    </Button>
                </form>
            </div>
        </div>
    )
}

Login.propTypes = {
    setToken: PropTypes.func.isRequired
  }