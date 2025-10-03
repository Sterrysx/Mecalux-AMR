#!/usr/bin/env node
/**
 * yaml2json.js
 * Usage: node tools/yaml2json.js <ID>
 * Example: node tools/yaml2json.js 1
 *
 * Reads:
 *   optimality/Layer1-2D-Mapping/Distributions/<ID>/map.yml
 * Writes:
 *   apps/Simulator/Distributions/Distribution<ID>.json
 *
 * Requires: npm i -D js-yaml
 */

const fs = require("fs");
const path = require("path");
const YAML = require("js-yaml");

function trim(s){ return s.replace(/^\s+|\s+$/g, ""); }
function splitLines(s){ return s.split(/\r?\n/); }

/** Normalize ascii_map into array of strings (top row first), remove spaces, pad/truncate to width/height */
function normalizeAscii(asciiAny, width, height){
  let lines = [];
  if (typeof asciiAny === "string") {
    lines = splitLines(asciiAny).map(trim).filter(Boolean);
  } else if (Array.isArray(asciiAny)) {
    lines = asciiAny.map(x => (typeof x === "string" ? trim(x) : "")).filter(Boolean);
  } else {
    throw new Error("ascii_map must be string or array of strings");
  }

  // Remove spaces between chars for “X X . .” style
  lines = lines.map(l => l.replace(/\s+/g, ""));

  // Enforce height by trimming/padding at TOP (since ascii is top-first)
  if (lines.length > height) lines = lines.slice(lines.length - height);
  if (lines.length < height) {
    const pad = Array.from({length: height - lines.length}, () => ".".repeat(width));
    lines = pad.concat(lines);
  }

  // Enforce width per row
  lines = lines.map(l => {
    if (l.length < width) return l + ".".repeat(width - l.length);
    if (l.length > width) return l.slice(0, width);
    return l;
  });

  return lines; // still top-first
}

/** Scan ASCII (top-first) to extract objects; returns {chargers, packets} with bottom-left origin coords */
function extractObjectsFromAscii(topFirstRows){
  const H = topFirstRows.length;
  const W = topFirstRows[0]?.length ?? 0;
  const chargers = [], packets = [];
  let ci = 1, pi = 1;

  for (let r = 0; r < H; r++){
    const row = topFirstRows[r];
    const y = (H - 1) - r; // bottom-left origin
    for (let x = 0; x < W; x++){
      const ch = row[x];
      if (ch === "C") chargers.push({ id: `C${ci++}`, x, y, capacity: 1 });
      if (ch === "P") packets.push({ id: `P${pi++}`, x, y, weight: 1.0 });
    }
  }
  return { chargers, packets };
}

function main(){
  const idArg = process.argv[2];
  if (!idArg) {
    console.error("Usage: node tools/yaml2json.js <ID>\nExample: node tools/yaml2json.js 1");
    process.exit(1);
  }

  // Accept "1" or "distribution1" etc. Extract trailing number for consistency.
  const matchNum = String(idArg).match(/(\d+)$/);
  if (!matchNum) {
    console.error("ID must contain a number (e.g. 1, distribution1).");
    process.exit(1);
  }
  const ID = matchNum[1];

  const ROOT = process.cwd(); // run from repo root
  const inPath  = path.resolve(ROOT, "optimality", "Layer1-2D-Mapping", "Distributions", ID, "map.yml");
  const outDir  = path.resolve(ROOT, "apps", "Simulator", "Distributions");
  const outPath = path.resolve(outDir, `Distribution${ID}.json`);

  if (!fs.existsSync(inPath)) {
    console.error("Input YAML not found:", inPath);
    process.exit(2);
  }
  if (!fs.existsSync(outDir)) fs.mkdirSync(outDir, { recursive: true });

  // Load YAML
  let doc;
  try {
    doc = YAML.load(fs.readFileSync(inPath, "utf8"));
  } catch (e) {
    console.error("Failed to parse YAML:", e.message);
    process.exit(3);
  }

  // Basic fields
  const meta = doc.meta || {};
  const width  = Number(meta.width  ?? 0);
  const height = Number(meta.height ?? 0);
  if (!width || !height) {
    console.error("meta.width/height missing or zero in YAML");
    process.exit(4);
  }

  // Normalize ascii_map (top-first)
  const asciiRows = normalizeAscii(doc.ascii_map, width, height);

  // Objects: prefer explicit doc.objects if present, else extract from ASCII
  let chargers = [], packets = [];
  if (doc.objects && (doc.objects.chargers || doc.objects.packets)) {
    chargers = (doc.objects.chargers || []).map(c => ({
      id: String(c.id ?? "C_"),
      x: Number(c.x ?? 0),
      y: Number(c.y ?? 0),
      capacity: Number(c.capacity ?? 1),
    }));
    packets = (doc.objects.packets || []).map(p => ({
      id: String(p.id ?? "P_"),
      x: Number(p.x ?? 0),
      y: Number(p.y ?? 0),
      weight: Number(p.weight ?? 1.0),
    }));
  } else {
    const auto = extractObjectsFromAscii(asciiRows);
    chargers = auto.chargers;
    packets  = auto.packets;
  }

  // Build JSON
  const out = {
    meta: {
      name: meta.name ?? `distribution${ID}`,
      version: meta.version ?? 1,
      author: meta.author ?? "",
      width, height,
      cell_size_m: Number(meta.cell_size_m ?? 0.5),
      origin: meta.origin ?? "bottom-left (x right, y up)",
      created: meta.created ?? ""
    },
    legend: doc.legend || {},
    ascii_map: asciiRows,     // top-first array of strings (no spaces)
    objects: { chargers, packets },
    notes: doc.notes ?? ""
  };

  fs.writeFileSync(outPath, JSON.stringify(out, null, 2));
  console.log(`✔ Wrote ${outPath}`);
  console.log(`  Chargers: ${chargers.length}  Packets: ${packets.length}`);
  console.log(`  From: ${inPath}`);
}

main();
