import React, { useState } from 'react';
import { render } from 'react-dom';
import { BrowserRouter, Route, Switch } from 'react-router-dom';

import Login from './components/Login';
import Dashboard from "./components/Main_dashboard";
import useToken from './components/util/useToken';

function setToken(userToken) {
  sessionStorage.setItem('token', JSON.stringify(userToken));
}

function getToken() {
  const tokenString = sessionStorage.getItem('token');
  const userToken = JSON.parse(tokenString!);
  return userToken?.token
}

function App() {
  const { token, setToken } = useToken();

  if(!token) {
    return <Login setToken={setToken} />
  }
  
  return (
    <div className="wrapper">
      <BrowserRouter>
        <Switch>
          <Route path="/Dashboard">
            <Dashboard />
          </Route>
        </Switch>
      </BrowserRouter>
    </div>
  )
}

const rootElement = document.getElementById("root")
render(<App />, rootElement)