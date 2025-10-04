import { StrictMode } from 'react'
import { createRoot } from 'react-dom/client'
import { BrowserRouter, Routes, Route, Navigate } from 'react-router-dom'
import './index.css'
import App from './App.tsx'
import GridEditor from './pages/GridEditor.tsx'
import IsometricView from './pages/IsometricView.tsx'
import Nav from './components/Nav.tsx'
import { DistributionProvider } from './contexts/DistributionContext.tsx'

createRoot(document.getElementById('root')!).render(
  <StrictMode>
    <DistributionProvider>
      <BrowserRouter>
        <div className="min-h-screen bg-slate-50">
          <Nav />
          <Routes>
            <Route path="/" element={<Navigate to="/editor" replace />} />
            <Route path="/editor" element={<GridEditor />} />
            <Route path="/isometric" element={<IsometricView />} />
            <Route path="/app" element={<App />} />
          </Routes>
        </div>
      </BrowserRouter>
    </DistributionProvider>
  </StrictMode>,
)
