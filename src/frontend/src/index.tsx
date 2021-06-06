import { render } from 'react-dom';
import { BrowserRouter, Route, Switch } from 'react-router-dom';

import Login from './components/Login';
import Dashboard from "./components/Main_dashboard";
import useToken from './components/util/useToken';

function App() {
  const { token, setToken } = useToken();

  if(!token) {
    return <Login setToken={setToken} />
  }
  
  return (
    <BrowserRouter>
        <Route path="/Dashboard">
          <Dashboard />
        </Route>
    </BrowserRouter>
  )
}

const rootElement = document.getElementById("root")

render(<App />, rootElement)