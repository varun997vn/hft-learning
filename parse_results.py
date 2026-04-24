import pandas as pd
import json

def process_data():
    files = {
        "local": "local_numa_results.csv",
        "cross": "cross_numa_results.csv"
    }
    
    results = {}
    
    for key, filename in files.items():
        try:
            df = pd.read_csv(filename)
            latencies = df['LatencyNs']
            
            percentiles = {
                "p50": latencies.quantile(0.50),
                "p90": latencies.quantile(0.90),
                "p99": latencies.quantile(0.99),
                "p99_9": latencies.quantile(0.999),
                "mean": latencies.mean(),
                "max": latencies.max(),
                "min": latencies.min()
            }
            
            # Subsample for histogram (too many points crashes browsers)
            # Take a random sample of 10000 points
            sample = latencies.sample(n=min(10000, len(latencies)), random_state=42).tolist()
            
            results[key] = {
                "percentiles": percentiles,
                "sample": sample
            }
            print(f"Processed {filename}")
        except Exception as e:
            print(f"Error processing {filename}: {e}")
            
    with open("docs-site/src/data/benchmark_results.json", "w") as f:
        json.dump(results, f)
        print("Successfully exported to docs-site/src/data/benchmark_results.json")

if __name__ == "__main__":
    process_data()
