#!/usr/bin/env python3
"""
Full Scenario Generator for Mecalux AMR

Combines POI generation and Task generation into a single workflow.
Generates a complete scenario with random charging zones, pickup/dropoff nodes, and tasks.

Usage:
    python scenario_generator.py --num-tasks 100 --seed 42
    python scenario_generator.py -n 50 --charging-clusters 2 --pickup-clusters 3
"""

import argparse
import subprocess
import sys
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(
        description='Generate complete AMR scenario (POIs + Tasks)'
    )
    
    # Task settings
    parser.add_argument(
        '-n', '--num-tasks',
        type=int,
        default=50,
        help='Number of tasks to generate (default: 50)'
    )
    
    # POI Cluster settings
    parser.add_argument(
        '--charging-clusters',
        type=int,
        default=None,
        help='Number of charging clusters (default: random 1-3)'
    )
    parser.add_argument(
        '--charging-per-cluster',
        type=int,
        default=None,
        help='Max charging nodes per cluster (default: random 1-2)'
    )
    parser.add_argument(
        '--pickup-clusters',
        type=int,
        default=None,
        help='Number of pickup clusters (default: random 1-4)'
    )
    parser.add_argument(
        '--pickup-per-cluster',
        type=int,
        default=None,
        help='Max pickup nodes per cluster (default: random 1-4)'
    )
    parser.add_argument(
        '--dropoff-clusters',
        type=int,
        default=None,
        help='Number of dropoff clusters (default: random 1-4)'
    )
    parser.add_argument(
        '--dropoff-per-cluster',
        type=int,
        default=None,
        help='Max dropoff nodes per cluster (default: random 1-4)'
    )
    
    # File paths
    parser.add_argument(
        '-m', '--map',
        type=str,
        default='../assets/map_layout.txt',
        help='Path to map layout file'
    )
    parser.add_argument(
        '--poi-output',
        type=str,
        default='../assets/poi_config.json',
        help='Output path for POI config (overwrites existing!)'
    )
    parser.add_argument(
        '--task-output',
        type=str,
        default='../../../api/set_of_tasks.json',
        help='Output path for tasks JSON'
    )
    
    # Common settings
    parser.add_argument(
        '-s', '--seed',
        type=int,
        default=None,
        help='Random seed for reproducibility'
    )
    parser.add_argument(
        '--robot-radius',
        type=float,
        default=0.3,
        help='Robot radius in meters (default: 0.3)'
    )
    parser.add_argument(
        '-q', '--quiet',
        action='store_true',
        help='Suppress detailed output'
    )
    parser.add_argument(
        '--skip-poi',
        action='store_true',
        help='Skip POI generation (use existing poi_config.json)'
    )
    
    args = parser.parse_args()
    
    script_dir = Path(__file__).parent
    
    print("=" * 70)
    print("          MECALUX AMR SCENARIO GENERATOR")
    print("=" * 70)
    
    # Step 1: Generate POIs (unless skipped)
    if not args.skip_poi:
        print("\n[Step 1/2] Generating POI Configuration...")
        print("-" * 50)
        
        poi_cmd = [
            sys.executable, str(script_dir / 'poi_generator.py'),
            '-m', args.map,
            '-o', args.poi_output,
            '--robot-radius', str(args.robot_radius),
        ]
        
        if args.seed is not None:
            poi_cmd.extend(['-s', str(args.seed)])
        if args.charging_clusters is not None:
            poi_cmd.extend(['--charging-clusters', str(args.charging_clusters)])
        if args.charging_per_cluster is not None:
            poi_cmd.extend(['--charging-per-cluster', str(args.charging_per_cluster)])
        if args.pickup_clusters is not None:
            poi_cmd.extend(['--pickup-clusters', str(args.pickup_clusters)])
        if args.pickup_per_cluster is not None:
            poi_cmd.extend(['--pickup-per-cluster', str(args.pickup_per_cluster)])
        if args.dropoff_clusters is not None:
            poi_cmd.extend(['--dropoff-clusters', str(args.dropoff_clusters)])
        if args.dropoff_per_cluster is not None:
            poi_cmd.extend(['--dropoff-per-cluster', str(args.dropoff_per_cluster)])
        if args.quiet:
            poi_cmd.append('-q')
        
        result = subprocess.run(poi_cmd)
        if result.returncode != 0:
            print("[ERROR] POI generation failed!")
            return 1
    else:
        print("\n[Step 1/2] Skipping POI generation (using existing config)")
    
    # Step 2: Generate Tasks
    print("\n[Step 2/2] Generating Tasks...")
    print("-" * 50)
    
    task_cmd = [
        sys.executable, str(script_dir / 'task_generator.py'),
        '-n', str(args.num_tasks),
        '-p', args.poi_output,
        '-o', args.task_output,
    ]
    
    if args.seed is not None:
        # Use seed+1 for tasks to get different randomness
        task_cmd.extend(['-s', str(args.seed + 1)])
    if args.quiet:
        task_cmd.append('-q')
    
    result = subprocess.run(task_cmd)
    if result.returncode != 0:
        print("[ERROR] Task generation failed!")
        return 1
    
    # Summary
    print("\n" + "=" * 70)
    print("          SCENARIO GENERATION COMPLETE")
    print("=" * 70)
    print(f"\nGenerated files:")
    if not args.skip_poi:
        print(f"  POI Config: {args.poi_output}")
    print(f"  Tasks:      {args.task_output}")
    print(f"\nRun Layer 2 test with:")
    print(f"  cd ../layer2 && ./build/test_layer2")
    print("=" * 70 + "\n")
    
    return 0


if __name__ == '__main__':
    exit(main())
