import { useState } from 'react';
import GridEditor from './pages/Editor/GridEditor';
import IsometricView from './pages/Isometric/IsometricView';
import TaskPlannerTest from './pages/TaskPlanner/TaskPlannerTest';

export default function App(){
  const [page, setPage] = useState<'grid'|'iso'|'task'>('grid');

  return (
    <div className="min-h-screen bg-slate-50 p-6">
      <div className="mx-auto max-w-[1100px]">
        <nav className="flex gap-2 mb-4">
          <button onClick={()=>setPage('grid')} className={`px-3 py-1 rounded ${page==='grid' ? 'bg-slate-900 text-white' : 'bg-white'}`}>Grid Editor</button>
          <button onClick={()=>setPage('iso')} className={`px-3 py-1 rounded ${page==='iso' ? 'bg-slate-900 text-white' : 'bg-white'}`}>Isometric View</button>
          <button onClick={()=>setPage('task')} className={`px-3 py-1 rounded ${page==='task' ? 'bg-slate-900 text-white' : 'bg-white'}`}>Task Planner</button>
        </nav>

        <main>
          {page==='grid' && <GridEditor />}
          {page==='iso' && <IsometricView />}
          {page==='task' && <TaskPlannerTest />}
        </main>
      </div>
    </div>
  );
}
