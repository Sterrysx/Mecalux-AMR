import { createContext, useContext, useState, useEffect } from 'react';
import type { ReactNode } from 'react';

export type Meta = { name: string; width: number; height: number; cell_size_m: number };
export type Packet = { id: string; x: number; y: number; weight?: number };
export type Charger = { id: string; x: number; y: number; capacity?: number };
export type DistFile = {
  meta: Meta;
  legend?: Record<string,string>;
  ascii_map: string[]; // top-first
  objects?: { packets?: Packet[]; chargers?: Charger[] };
  notes?: string;
};

interface DistributionContextType {
  distribution: DistFile | null;
  grid: string[][];
  updateGrid: (newGrid: string[][]) => void;
  saveDistribution: () => Promise<void>;
  isLoading: boolean;
  error: string | null;
}

const DistributionContext = createContext<DistributionContextType | undefined>(undefined);

export function DistributionProvider({ children }: { children: ReactNode }) {
  const [distribution, setDistribution] = useState<DistFile | null>(null);
  const [grid, setGrid] = useState<string[][]>([]);
  const [isLoading, setIsLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  // Load initial distribution
  useEffect(() => {
    loadDistribution();
  }, []);

  const loadDistribution = async () => {
    try {
      setIsLoading(true);
      setError(null);
      
      const response = await fetch("/distributions/Distribution1.json");
      const data: DistFile = await response.json();
      
      const W = data.meta.width;
      const H = data.meta.height;
      
      // Normalize ascii rows (top-first, no spaces)
      const rows = data.ascii_map.map(r => r.replace(/\s+/g,""));
      
      // Convert to bottom-left origin 2D grid[y][x]
      const g: string[][] = Array.from({length: H}, () => Array(W).fill("."));
      for (let r = 0; r < rows.length; r++) {
        const y = (H - 1) - r;
        for (let x = 0; x < Math.min(W, rows[r].length); x++) {
          g[y][x] = rows[r][x];
        }
      }
      
      setDistribution(data);
      setGrid(g);
    } catch (err) {
      setError(String(err));
    } finally {
      setIsLoading(false);
    }
  };

  const updateGrid = (newGrid: string[][]) => {
    setGrid(newGrid);
    
    // Auto-update distribution data
    if (distribution) {
      const W = distribution.meta.width;
      const H = distribution.meta.height;
      
      // Convert bottom-left grid back to top-first ascii_map
      const rowsTop: string[] = [];
      for (let r = H-1; r >= 0; r--) {
        rowsTop.push(newGrid[r].join(""));
      }
      
      // Extract objects from grid
      const chargers: Charger[] = [];
      const packets: Packet[] = [];
      let ci = 1, pi = 1;
      
      for (let y = 0; y < H; y++) {
        for (let x = 0; x < W; x++) {
          const ch = newGrid[y][x];
          if (ch === "C") chargers.push({ id: `C${ci++}`, x, y, capacity: 1 });
          if (ch === "P") packets.push({ id: `P${pi++}`, x, y, weight: 1 });
        }
      }
      
      const updatedDistribution: DistFile = {
        ...distribution,
        ascii_map: rowsTop,
        objects: { chargers, packets }
      };
      
      setDistribution(updatedDistribution);
    }
  };

  const saveDistribution = async () => {
    if (!distribution) return;
    
    try {
      // In a real app, you'd save to a backend
      // For now, we'll just trigger a download or use localStorage
      const dataStr = JSON.stringify(distribution, null, 2);
      
      // Save to localStorage as backup
      localStorage.setItem('currentDistribution', dataStr);
      
      // Also trigger a download
      const blob = new Blob([dataStr], { type: "application/json" });
      const a = document.createElement("a");
      a.href = URL.createObjectURL(blob);
      a.download = `${distribution.meta.name || "distribution"}-edited.json`;
      a.click();
      URL.revokeObjectURL(a.href);
      
      console.log('Distribution saved successfully');
    } catch (err) {
      console.error('Failed to save distribution:', err);
      throw err;
    }
  };

  return (
    <DistributionContext.Provider value={{
      distribution,
      grid,
      updateGrid,
      saveDistribution,
      isLoading,
      error
    }}>
      {children}
    </DistributionContext.Provider>
  );
}

export function useDistribution() {
  const context = useContext(DistributionContext);
  if (context === undefined) {
    throw new Error('useDistribution must be used within a DistributionProvider');
  }
  return context;
}