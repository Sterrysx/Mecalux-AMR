import { NavLink } from "react-router-dom";
import { useTheme } from "../contexts/ThemeContext";

export default function Nav() {
  const { darkMode, toggleDarkMode } = useTheme();
  
  const linkBase =
    "px-3 py-2 rounded-md text-sm font-medium border border-transparent";
  const active = darkMode ? "bg-slate-700 text-white" : "bg-slate-900 text-white";
  const idle = darkMode 
    ? "bg-slate-800 text-slate-200 border-slate-700 hover:bg-slate-700" 
    : "bg-white text-slate-900 border-slate-300 hover:bg-slate-50";

  return (
    <header className={`sticky top-0 z-10 backdrop-blur border-b transition-colors ${
      darkMode ? 'bg-slate-800/90 border-slate-700' : 'bg-white/90 border-slate-200'
    }`}>
      <div className="mx-auto max-w-[1100px] px-4 py-3 flex items-center gap-3">
        <div className={`text-lg font-semibold ${darkMode ? 'text-white' : 'text-slate-900'}`}>AMR Simulator</div>
        <nav className="ml-auto flex gap-2 items-center">
          <NavLink to="/editor" className={({isActive})=>`${linkBase} ${isActive?active:idle}`}>Editor</NavLink>
          <NavLink to="/isometric" className={({isActive})=>`${linkBase} ${isActive?active:idle}`}>Isometric</NavLink>
          <NavLink to="/planner" className={({isActive})=>`${linkBase} ${isActive?active:idle}`}>Task Planner</NavLink>
          <NavLink to="/realtime" className={({isActive})=>`${linkBase} ${isActive?active:idle}`}>Real-Time</NavLink>
          
          {/* Cool Animated Theme Toggle Switch */}
          <button
            onClick={toggleDarkMode}
            className={`relative w-16 h-8 rounded-full transition-all duration-500 transform hover:scale-105 border ${
              darkMode 
                ? 'bg-slate-700 border-slate-600' 
                : 'bg-slate-200 border-slate-300'
            }`}
            title={darkMode ? "Switch to day mode" : "Switch to night mode"}
            aria-label="Toggle theme"
          >
            {/* Toggle Circle with Icon */}
            <div
              className={`absolute top-0.5 left-0.5 w-7 h-7 rounded-full transition-all duration-500 transform flex items-center justify-center ${
                darkMode 
                  ? 'translate-x-8 bg-slate-900 border-2 border-slate-600' 
                  : 'translate-x-0 bg-white border-2 border-slate-300'
              }`}
            >
              {darkMode ? (
                // Moon Icon
                <svg className="w-4 h-4 text-yellow-400 animate-pulse" fill="currentColor" viewBox="0 0 20 20">
                  <path d="M17.293 13.293A8 8 0 016.707 2.707a8.001 8.001 0 1010.586 10.586z" />
                </svg>
              ) : (
                // Sun Icon with rotating animation
                <svg className="w-4 h-4 text-yellow-600 animate-spin-slow" fill="currentColor" viewBox="0 0 20 20">
                  <path fillRule="evenodd" d="M10 2a1 1 0 011 1v1a1 1 0 11-2 0V3a1 1 0 011-1zm4 8a4 4 0 11-8 0 4 4 0 018 0zm-.464 4.95l.707.707a1 1 0 001.414-1.414l-.707-.707a1 1 0 00-1.414 1.414zm2.12-10.607a1 1 0 010 1.414l-.706.707a1 1 0 11-1.414-1.414l.707-.707a1 1 0 011.414 0zM17 11a1 1 0 100-2h-1a1 1 0 100 2h1zm-7 4a1 1 0 011 1v1a1 1 0 11-2 0v-1a1 1 0 011-1zM5.05 6.464A1 1 0 106.465 5.05l-.708-.707a1 1 0 00-1.414 1.414l.707.707zm1.414 8.486l-.707.707a1 1 0 01-1.414-1.414l.707-.707a1 1 0 011.414 1.414zM4 11a1 1 0 100-2H3a1 1 0 000 2h1z" clipRule="evenodd" />
                </svg>
              )}
            </div>
            
            {/* Background decorations */}
            {darkMode ? (
              // Stars in night mode
              <div className="absolute inset-0 flex items-center justify-start pl-2 pointer-events-none">
                <div className="flex gap-0.5">
                  <span className="text-yellow-300 text-xs animate-twinkle">✨</span>
                  <span className="text-yellow-200 text-[8px] animate-twinkle-delayed">⭐</span>
                </div>
              </div>
            ) : (
              // Clouds in day mode
              <div className="absolute inset-0 flex items-center justify-end pr-2 pointer-events-none">
                <span className="text-slate-400 text-xs opacity-60">☁️</span>
              </div>
            )}
          </button>
        </nav>
      </div>
    </header>
  );
}
