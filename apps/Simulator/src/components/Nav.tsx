import { NavLink } from "react-router-dom";

export default function Nav() {
  const linkBase =
    "px-3 py-2 rounded-md text-sm font-medium border border-transparent";
  const active = "bg-slate-900 text-white";
  const idle   = "bg-white text-slate-900 border-slate-300 hover:bg-slate-50";

  return (
    <header className="sticky top-0 z-10 bg-white/90 backdrop-blur border-b border-slate-200">
      <div className="mx-auto max-w-[1100px] px-4 py-3 flex items-center gap-3">
        <div className="text-lg font-semibold">AMR Simulator</div>
        <nav className="ml-auto flex gap-2">
          <NavLink to="/editor" className={({isActive})=>`${linkBase} ${isActive?active:idle}`}>Editor</NavLink>
          <NavLink to="/isometric" className={({isActive})=>`${linkBase} ${isActive?active:idle}`}>Isometric</NavLink>
        </nav>
      </div>
    </header>
  );
}
