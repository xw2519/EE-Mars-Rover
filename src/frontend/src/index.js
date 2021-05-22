import React from "react";
import { render } from 'react-dom';

import Dashboard from "./components/dashboard";

function App() {
  
  return (
    
    <Dashboard />
  )
}

const rootElement = document.getElementById("root")
render(<App />, rootElement)