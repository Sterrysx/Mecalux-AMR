#!/usr/bin/env python3
"""
Task Generator for Mecalux AMR Fleet Manager

Generates random pickup/dropoff tasks from POI configuration.
Reads existing POI config and creates a set_of_tasks.json file.

Usage:
    python task_generator.py --num-tasks 50 --output ../../api/set_of_tasks.json
    python task_generator.py -n 100 -o tasks.json --seed 42
"""

import json
import random
import argparse
from pathlib import Path
from typing import List, Dict, Tuple


def load_poi_config(filepath: str) -> Dict:
    """Load POI configuration from JSON file."""
    with open(filepath, 'r') as f:
        return json.load(f)


def extract_nodes_by_type(poi_config: Dict) -> Tuple[List[str], List[str]]:
    """
    Extract pickup and dropoff node IDs from POI config.
    
    Returns:
        Tuple of (pickup_nodes, dropoff_nodes) lists
    """
    pickup_nodes = []
    dropoff_nodes = []
    
    for poi in poi_config.get('poi', []):
        poi_id = poi.get('id', '')
        poi_type = poi.get('type', '').upper()
        is_active = poi.get('active', True)
        
        if not is_active:
            continue
            
        if poi_type == 'PICKUP':
            pickup_nodes.append(poi_id)
        elif poi_type == 'DROPOFF':
            dropoff_nodes.append(poi_id)
    
    return pickup_nodes, dropoff_nodes


def generate_tasks(
    pickup_nodes: List[str],
    dropoff_nodes: List[str],
    num_tasks: int,
    seed: int = None
) -> List[Dict]:
    """
    Generate random tasks pairing pickup nodes to dropoff nodes.
    
    Args:
        pickup_nodes: List of pickup POI IDs
        dropoff_nodes: List of dropoff POI IDs
        num_tasks: Number of tasks to generate
        seed: Random seed for reproducibility
    
    Returns:
        List of task dictionaries
    """
    if seed is not None:
        random.seed(seed)
    
    if not pickup_nodes:
        raise ValueError("No pickup nodes available")
    if not dropoff_nodes:
        raise ValueError("No dropoff nodes available")
    
    tasks = []
    for i in range(1, num_tasks + 1):
        source = random.choice(pickup_nodes)
        destination = random.choice(dropoff_nodes)
        
        tasks.append({
            "id": i,
            "source": source,
            "destination": destination
        })
    
    return tasks


def save_tasks(tasks: List[Dict], output_path: str):
    """Save tasks to JSON file."""
    output = {
        "tasks": tasks
    }
    
    with open(output_path, 'w') as f:
        json.dump(output, f, indent=2)
    
    print(f"[TaskGenerator] Saved {len(tasks)} tasks to: {output_path}")


def print_summary(pickup_nodes: List[str], dropoff_nodes: List[str], tasks: List[Dict]):
    """Print generation summary."""
    print("\n" + "=" * 60)
    print("           TASK GENERATOR SUMMARY")
    print("=" * 60)
    print(f"\nAvailable Nodes:")
    print(f"  Pickup nodes:  {pickup_nodes}")
    print(f"  Dropoff nodes: {dropoff_nodes}")
    print(f"\nGenerated Tasks: {len(tasks)}")
    
    # Count task distribution
    source_counts = {}
    dest_counts = {}
    for task in tasks:
        src = task['source']
        dst = task['destination']
        source_counts[src] = source_counts.get(src, 0) + 1
        dest_counts[dst] = dest_counts.get(dst, 0) + 1
    
    print(f"\nSource Distribution:")
    for node, count in sorted(source_counts.items()):
        print(f"  {node}: {count} tasks ({100*count/len(tasks):.1f}%)")
    
    print(f"\nDestination Distribution:")
    for node, count in sorted(dest_counts.items()):
        print(f"  {node}: {count} tasks ({100*count/len(tasks):.1f}%)")
    
    print("\nSample Tasks (first 5):")
    for task in tasks[:5]:
        print(f"  Task {task['id']}: {task['source']} → {task['destination']}")
    if len(tasks) > 5:
        print(f"  ... and {len(tasks) - 5} more")
    print("=" * 60 + "\n")


def main():
    parser = argparse.ArgumentParser(
        description='Generate random pickup/dropoff tasks from POI configuration'
    )
    parser.add_argument(
        '-n', '--num-tasks',
        type=int,
        default=50,
        help='Number of tasks to generate (default: 50)'
    )
    parser.add_argument(
        '-p', '--poi-config',
        type=str,
        default='../assets/poi_config.json',
        help='Path to POI configuration file'
    )
    parser.add_argument(
        '-o', '--output',
        type=str,
        default='../../../api/set_of_tasks.json',
        help='Output path for generated tasks JSON'
    )
    parser.add_argument(
        '-s', '--seed',
        type=int,
        default=None,
        help='Random seed for reproducibility'
    )
    parser.add_argument(
        '-q', '--quiet',
        action='store_true',
        help='Suppress summary output'
    )
    
    args = parser.parse_args()
    
    # Resolve paths relative to script location
    script_dir = Path(__file__).parent
    poi_path = script_dir / args.poi_config
    output_path = script_dir / args.output
    
    print(f"[TaskGenerator] Loading POI config from: {poi_path}")
    
    # Load POI configuration
    try:
        poi_config = load_poi_config(poi_path)
    except FileNotFoundError:
        print(f"[TaskGenerator] ERROR: POI config not found: {poi_path}")
        return 1
    except json.JSONDecodeError as e:
        print(f"[TaskGenerator] ERROR: Invalid JSON in POI config: {e}")
        return 1
    
    # Extract nodes
    pickup_nodes, dropoff_nodes = extract_nodes_by_type(poi_config)
    
    if not pickup_nodes or not dropoff_nodes:
        print(f"[TaskGenerator] ERROR: Need at least one pickup and one dropoff node")
        print(f"  Found: {len(pickup_nodes)} pickup, {len(dropoff_nodes)} dropoff")
        return 1
    
    # Generate tasks
    print(f"[TaskGenerator] Generating {args.num_tasks} tasks...")
    tasks = generate_tasks(pickup_nodes, dropoff_nodes, args.num_tasks, args.seed)
    
    # Save tasks
    output_path.parent.mkdir(parents=True, exist_ok=True)
    save_tasks(tasks, output_path)
    
    # Print summary
    if not args.quiet:
        print_summary(pickup_nodes, dropoff_nodes, tasks)
    
    return 0


if __name__ == '__main__':
    exit(main())
