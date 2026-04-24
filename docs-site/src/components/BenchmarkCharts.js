import React from 'react';
import BrowserOnly from '@docusaurus/BrowserOnly';
import benchmarkData from '../data/benchmark_results.json';

const BenchmarkCharts = () => {
  return (
    <BrowserOnly fallback={<div>Loading charts...</div>}>
      {() => {
        const Plot = require('react-plotly.js').default;

        const localSample = benchmarkData.local.sample;
        const crossSample = benchmarkData.cross.sample;

        // Histogram Data
        const histogramData = [
          {
            x: localSample,
            type: 'histogram',
            name: 'Local NUMA',
            opacity: 0.7,
            marker: { color: 'blue' },
          },
          {
            x: crossSample,
            type: 'histogram',
            name: 'Cross NUMA',
            opacity: 0.7,
            marker: { color: 'red' },
          }
        ];

        const histogramLayout = {
          title: 'Latency Distribution (Histogram)',
          xaxis: { title: 'Latency (ns)' },
          yaxis: { title: 'Frequency' },
          barmode: 'overlay',
        };

        // Box Plot Data
        const boxPlotData = [
          {
            y: localSample,
            type: 'box',
            name: 'Local NUMA',
            marker: { color: 'blue' },
          },
          {
            y: crossSample,
            type: 'box',
            name: 'Cross NUMA',
            marker: { color: 'red' },
          }
        ];

        const boxPlotLayout = {
          title: 'Latency Variance (Box Plot)',
          yaxis: { title: 'Latency (ns)' },
        };

        return (
          <div>
            <h3>Interactive Benchmark Analysis</h3>
            <div style={{ display: 'flex', flexDirection: 'column', gap: '2rem' }}>
              <Plot data={histogramData} layout={histogramLayout} useResizeHandler style={{ width: '100%', height: '400px' }} />
              <Plot data={boxPlotData} layout={boxPlotLayout} useResizeHandler style={{ width: '100%', height: '400px' }} />
            </div>
            
            <h3>Percentiles Summary</h3>
            <table style={{ width: '100%', textAlign: 'left' }}>
              <thead>
                <tr>
                  <th>Metric</th>
                  <th>Local NUMA (ns)</th>
                  <th>Cross NUMA (ns)</th>
                </tr>
              </thead>
              <tbody>
                <tr>
                  <td>Mean</td>
                  <td>{benchmarkData.local.percentiles.mean.toFixed(2)}</td>
                  <td>{benchmarkData.cross.percentiles.mean.toFixed(2)}</td>
                </tr>
                <tr>
                  <td>p50 (Median)</td>
                  <td>{benchmarkData.local.percentiles.p50.toFixed(2)}</td>
                  <td>{benchmarkData.cross.percentiles.p50.toFixed(2)}</td>
                </tr>
                <tr>
                  <td>p90</td>
                  <td>{benchmarkData.local.percentiles.p90.toFixed(2)}</td>
                  <td>{benchmarkData.cross.percentiles.p90.toFixed(2)}</td>
                </tr>
                <tr>
                  <td>p99</td>
                  <td>{benchmarkData.local.percentiles.p99.toFixed(2)}</td>
                  <td>{benchmarkData.cross.percentiles.p99.toFixed(2)}</td>
                </tr>
                <tr>
                  <td>p99.9</td>
                  <td>{benchmarkData.local.percentiles.p99_9.toFixed(2)}</td>
                  <td>{benchmarkData.cross.percentiles.p99_9.toFixed(2)}</td>
                </tr>
                <tr>
                  <td>Max</td>
                  <td>{benchmarkData.local.percentiles.max.toFixed(2)}</td>
                  <td>{benchmarkData.cross.percentiles.max.toFixed(2)}</td>
                </tr>
              </tbody>
            </table>
          </div>
        );
      }}
    </BrowserOnly>
  );
};

export default BenchmarkCharts;
