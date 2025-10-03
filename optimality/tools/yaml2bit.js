#!/usr/bin/env node
/**
 * yaml2bit
 * Usage: tools/yaml2bit <ID>
 * Example: tools/yaml2bit 1
 *
 * Reads:
 *   Optimality/Layer1-2D-Mapping/Distributions/<ID>/map.yml
 * Writes (text file, top-first):
 *   Optimality/Layer1-2D-Mapping/Distributions/<ID>/bit_matrix.bit
 *
 * Conventions:
 *   Blocked chars → '1': X, #, P
 *   All others    → '0'
 *
 * Requires: npm i -D js-yaml
 */

const fs = require("fs");
const path = require("path");
const YAML = require("js-yaml");

function trim(s){ return s.replace(/^\s+|\s+$/g, ""); }
function splitLines(s){ return s.split(/\r?\n/); }

function normalizeAscii(asciiAny, width, height){
  let lines = [];
  if (typeof asciiAny === "string") {
    lines = splitLines(asciiAny).map(trim).filter(Boolean);
  } else if (Array.isArray(asciiAny)) {
    lines = asciiAny.map((x) => (typeof x === "string" ? trim(x) : "")).filter(Boolean);
  } else {
    throw new Error("ascii_map must be a string or array of strings");
  }

  // Remove spaces between symbols ("X X ." -> "XX.")
  lines = lines.map(l => l.replace(/\s+/g, ""));

  // Enforce height: ascii is top-first → trim/pad at TOP
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

  return lines; // top-first rows
}

function main(){
  const idArg = process.argv[2];
  if (!idArg) {
    console.error("Usage: tools/yaml2bit <ID>\nExample: tools/yaml2bit 1");
    process.exit(1);
  }

  const matchNum = String(idArg).match(/(\d+)$/);
  if (!matchNum) {
    console.error("ID must contain a number (e.g. 1, distribution1).");
    process.exit(1);
  }
  const ID = matchNum[1];

  const ROOT = process.cwd(); // run from repo root
  const ymlDir  = path.resolve(ROOT, "Optimality", "Layer1-2D-Mapping", "Distributions", ID);
  const inPath  = path.join(ymlDir, "map.yml");
  const outPath = path.join(ymlDir, "bit_matrix.bit");

  if (!fs.existsSync(inPath)) {
    console.error("YAML not found:", inPath);
    process.exit(2);
  }

  let doc;
  try {
    doc = YAML.load(fs.readFileSync(inPath, "utf8"));
  } catch (e) {
    console.error("Failed to parse YAML:", e.message);
    process.exit(3);
  }

  const meta = doc.meta || {};
  const width  = Number(meta.width  ?? 0);
  const height = Number(meta.height ?? 0);
  if (!width || !height) {
    console.error("meta.width/height missing or zero in YAML");
    process.exit(4);
  }

  const asciiRowsTop = normalizeAscii(doc.ascii_map, width, height);

  // Convention: blocked set = X, #, P
  const blocked = new Set(["X", "#", "P"]);

  // Build text bit matrix (top-first)
  const lines = asciiRowsTop.map(row =>
    row.split("").map(ch => (blocked.has(ch) ? "1" : "0")).join("")
  );

  // Write text file (10 lines for 10x10, etc.)
  fs.writeFileSync(outPath, lines.join("\n") + "\n", "utf8");

  console.log(`✔ Wrote ${outPath}`);
  console.log(`  Size: ${height} lines × ${width} chars (top-first)`);
}

main();
