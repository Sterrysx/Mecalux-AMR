type BrushKey = 'EMPTY' | 'PATH' | 'OBSTACLE' | 'RESTRICTED' | 'CHARGER' | 'PACKET';

export default function Palette({
  items,
  selected,
  onSelect,
}: {
  items: Array<[BrushKey, string, string]>;
  selected?: BrushKey;
  onSelect: (k: BrushKey) => void;
}) {
  return (
    <div className="flex flex-wrap items-center gap-2 rounded-lg border border-slate-300 bg-white p-3">
      {items.map(([key, ch, color]) => (
        <button
          key={key}
          onClick={() => onSelect(key)}
          className={`flex items-center gap-2 rounded-md border px-3 py-1 text-sm ${selected === key ? 'border-slate-900' : 'border-slate-300 hover:border-slate-400'}`}
          title={`${key} (${ch})`}
        >
          <span className="inline-block h-4 w-4 rounded border border-slate-300" style={{ background: color }} />
          <span>{key}</span>
        </button>
      ))}
    </div>
  );
}
