import React from 'react';
import './Login.css';
import { TextInputField, Button } from 'evergreen-ui'

interface LoginProps {
    setToken: any; 
}

export default function Login(Props: LoginProps) {
    return(
        <div className="login-wrapper">
            <div className="login_title_1"> 
                Mars Rover Command Center 
            </div>

            <div className="login_title_2"> 
                Login 
            </div>

            <div className="login_field"> 
                <form>

                    <TextInputField
                        placeholder="Username"
                        inputHeight={50}
                        inputWidth={300}
                    />
    
                    <TextInputField
                        placeholder="Password"
                        inputHeight={50}
                        inputWidth={300}
                    />

                    <Button marginTop={60} appearance="primary" size="large">
                        Login
                    </Button>
                </form>
            </div>
        </div>
    )
}