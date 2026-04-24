import pandas as pd
import sys

def validate_csv(filename):
    print(f"Validating {filename}...")
    try:
        df = pd.read_csv(filename)
        rows = len(df)
        print(f"Number of rows: {rows}")
        
        if rows != 1000000:
            print("WARNING: Expected 1,000,000 rows!")
            
        mean_latency = df['LatencyNs'].mean()
        min_latency = df['LatencyNs'].min()
        max_latency = df['LatencyNs'].max()
        
        print(f"Mean Latency: {mean_latency:.2f} ns")
        print(f"Min Latency:  {min_latency:.2f} ns")
        print(f"Max Latency:  {max_latency:.2f} ns\n")
        return True
    except Exception as e:
        print(f"Validation failed: {e}")
        return False

if __name__ == "__main__":
    validate_csv("local_numa_results.csv")
    validate_csv("cross_numa_results.csv")
