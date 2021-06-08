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
    <div className="wrapper">
      <BrowserRouter>
        <Switch>
        <Route path="/">
          <Dashboard />
          </Route>
        </Switch>
      </BrowserRouter>
    </div>
  )
}

const rootElement = document.getElementById("root")

render(<App />, rootElement)