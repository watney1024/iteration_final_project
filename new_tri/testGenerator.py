#!/usr/bin/env python3
"""
Test case generator for tridiagonal systems
Generates random diagonally dominant tridiagonal matrices
"""
import random
import pathlib
import argparse

def gen_system(n: int, rng: random.Random):
    """
    Generate a diagonally dominant tridiagonal system.
    Returns: (a, b, c, d) where a is sub-diagonal, b is main diagonal, 
             c is super-diagonal, and d is right-hand side
    """
    # Generate main diagonal (make it dominant)
    b = [rng.uniform(5.0, 10.0) for _ in range(n)]
    
    # Generate sub-diagonal (a[0] is unused)
    a = [0.0] + [rng.uniform(-2.0, 2.0) for _ in range(n-1)]
    
    # Generate super-diagonal (c[n-1] is unused)
    c = [rng.uniform(-2.0, 2.0) for _ in range(n-1)] + [0.0]
    
    # Ensure diagonal dominance: |b[i]| > |a[i]| + |c[i]|
    for i in range(n):
        row_sum = abs(a[i]) + abs(c[i])
        if abs(b[i]) <= row_sum:
            b[i] = (row_sum + 1.0) * (1.0 if b[i] >= 0 else -1.0)
    
    # Generate right-hand side
    d = [rng.uniform(-10.0, 10.0) for _ in range(n)]
    
    return a, b, c, d


def write_case(path: pathlib.Path, n: int, seed):
    """Write a single test case to file."""
    rng = random.Random(seed)
    a, b, c, d = gen_system(n, rng)
    
    path.parent.mkdir(parents=True, exist_ok=True)
    with open(path, 'w') as f:
        # Write size
        f.write(f"{n}\n")
        
        # Write main diagonal
        f.write(" ".join(f"{val:.10f}" for val in b) + "\n")
        
        # Write sub-diagonal (skip a[0])
        f.write(" ".join(f"{val:.10f}" for val in a[1:]) + "\n")
        
        # Write super-diagonal (skip c[n-1])
        f.write(" ".join(f"{val:.10f}" for val in c[:-1]) + "\n")
        
        # Write right-hand side
        f.write(" ".join(f"{val:.10f}" for val in d) + "\n")
    
    print(f"Generated test case: {path} (N={n})")


def main():
    parser = argparse.ArgumentParser(description="Generate tridiagonal system test cases")
    parser.add_argument('-n', '--size', type=int, default=1024, 
                        help='Size of the tridiagonal system (default: 1024)')
    parser.add_argument('-o', '--output', type=str, default='inputs/test_input.txt',
                        help='Output file path (default: inputs/test_input.txt)')
    parser.add_argument('-s', '--seed', type=int, default=None,
                        help='Random seed (default: None)')
    
    args = parser.parse_args()
    
    seed = args.seed if args.seed is not None else random.randint(0, 1000000)
    output_path = pathlib.Path(args.output)
    
    write_case(output_path, args.size, seed)
    print(f"Seed used: {seed}")


if __name__ == "__main__":
    main()
