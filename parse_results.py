import pandas as pd
import numpy as np
import json

def process_data():
    files = {
        "local": "local_numa_results.csv",
        "cross": "cross_numa_results.csv"
    }
    
    results = {}
    
    # Define histogram bins in log10 space (e.g., 10^2 to 10^8 nanoseconds)
    log_bins = np.arange(2.0, 8.0, 0.05)
    
    for key, filename in files.items():
        try:
            df = pd.read_csv(filename)
            latencies = df['LatencyNs']
            
            # 1. Percentiles (Summary)
            percentiles = {
                "p50": latencies.quantile(0.50),
                "p90": latencies.quantile(0.90),
                "p99": latencies.quantile(0.99),
                "p99_9": latencies.quantile(0.999),
                "mean": latencies.mean(),
                "max": latencies.max(),
                "min": latencies.min()
            }
            
            # 2. Box Plot Precalculation (No outliers to save rendering time)
            q1 = latencies.quantile(0.25)
            median = latencies.median()
            q3 = latencies.quantile(0.75)
            iqr = q3 - q1
            lowerfence = max(latencies.min(), q1 - 1.5 * iqr)
            upperfence = min(latencies.max(), q3 + 1.5 * iqr)
            
            box_plot = {
                "min": latencies.min(),
                "q1": q1,
                "median": median,
                "q3": q3,
                "max": latencies.max(),
                "lowerfence": lowerfence,
                "upperfence": upperfence
            }
            
            # 3. Histogram Precalculation
            # We calculate histogram counts in log10 space. 
            log_latencies = np.log10(latencies)
            counts, edges = np.histogram(log_latencies, bins=log_bins)
            
            # The frontend 'bar' chart in Plotly uses x (centers or strings) and y (counts)
            # Since Plotly xaxis type='log' natively transforms the linear values to log positions,
            # we should provide the raw linear coordinates for the bin centers!
            # So if a bin is from 10^4 to 10^4.05, the center is 10^4.025
            bin_centers_log = (edges[:-1] + edges[1:]) / 2
            bin_centers_linear = (10 ** bin_centers_log).tolist()
            
            histogram = {
                "x": bin_centers_linear,
                "y": counts.tolist()
            }
            
            results[key] = {
                "percentiles": percentiles,
                "box_plot": box_plot,
                "histogram": histogram
            }
            print(f"Processed {filename}")
        except Exception as e:
            print(f"Error processing {filename}: {e}")
            
    with open("docs-site/src/data/benchmark_results.json", "w") as f:
        json.dump(results, f)
        print("Successfully exported to docs-site/src/data/benchmark_results.json")

if __name__ == "__main__":
    process_data()
