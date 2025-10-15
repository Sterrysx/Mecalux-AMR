import type { ReactNode } from 'react';
import { useTheme } from '../contexts/ThemeContext';

export default function AppContainer({ children }: { children: ReactNode }) {
  const { darkMode } = useTheme();
  
  return (
    <div className={`min-h-screen transition-colors ${darkMode ? 'bg-slate-900' : 'bg-slate-50'}`}>
      {children}
    </div>
  );
}
