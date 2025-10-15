import { StrictMode } from 'react'
import { createRoot } from 'react-dom/client'
import { BrowserRouter, Routes, Route, Navigate } from 'react-router-dom'
import './index.css'
import App from './App.tsx'
import GridEditor from './pages/GridEditor.tsx'
import IsometricView from './pages/IsometricView.tsx'
import TaskPlanner from './pages/TaskPlannerTest.tsx'
import SimpleTest from './pages/SimpleTest.tsx'
import Nav from './components/Nav.tsx'
import AppContainer from './components/AppContainer.tsx'
import { DistributionProvider } from './contexts/DistributionContext.tsx'
import { ThemeProvider } from './contexts/ThemeContext.tsx'

createRoot(document.getElementById('root')!).render(
  <StrictMode>
    <ThemeProvider>
      <DistributionProvider>
        <BrowserRouter>
          <AppContainer>
            <Nav />
            <Routes>
              <Route path="/" element={<Navigate to="/editor" replace />} />
              <Route path="/editor" element={<GridEditor />} />
              <Route path="/isometric" element={<IsometricView />} />
              <Route path="/planner" element={<TaskPlanner />} />
              <Route path="/simple" element={<SimpleTest />} />
              <Route path="/app" element={<App />} />
            </Routes>
          </AppContainer>
        </BrowserRouter>
      </DistributionProvider>
    </ThemeProvider>
  </StrictMode>,
)
