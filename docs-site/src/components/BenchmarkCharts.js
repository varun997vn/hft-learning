import React from 'react';
import BrowserOnly from '@docusaurus/BrowserOnly';
import benchmarkData from '../data/benchmark_results.json';

const BenchmarkCharts = () => {
  return (
    <BrowserOnly fallback={<div>Loading charts...</div>}>
      {() => {
        const Plot = require('react-plotly.js').default;

        // Histogram Data
        const histogramData = [
          {
            x: benchmarkData.local.histogram.x,
            y: benchmarkData.local.histogram.y,
            type: 'bar',
            name: 'Local NUMA',
            opacity: 0.7,
            marker: { color: 'blue' }
          },
          {
            x: benchmarkData.cross.histogram.x,
            y: benchmarkData.cross.histogram.y,
            type: 'bar',
            name: 'Cross NUMA',
            opacity: 0.7,
            marker: { color: 'red' }
          }
        ];

        const histogramLayout = {
          title: { text: 'Latency Distribution (Histogram)', font: { size: 20 } },
          xaxis: { 
            title: { text: 'Latency (ns) - LOG SCALE', font: { size: 16 } },
            type: 'log',
            tickfont: { size: 14 }
          },
          yaxis: { 
            title: { text: 'Frequency (Number of Occurrences)', font: { size: 16 } },
            tickfont: { size: 14 }
          },
          barmode: 'overlay',
          margin: { l: 80, b: 80, t: 60, r: 20 }
        };

        // Box Plot Data (Pre-calculated without outliers for lightning fast rendering)
        const boxPlotData = [
          {
            type: 'box',
            name: 'Local NUMA',
            marker: { color: 'blue' },
            q1: [benchmarkData.local.box_plot.q1],
            median: [benchmarkData.local.box_plot.median],
            q3: [benchmarkData.local.box_plot.q3],
            lowerfence: [benchmarkData.local.box_plot.lowerfence],
            upperfence: [benchmarkData.local.box_plot.upperfence]
          },
          {
            type: 'box',
            name: 'Cross NUMA',
            marker: { color: 'red' },
            q1: [benchmarkData.cross.box_plot.q1],
            median: [benchmarkData.cross.box_plot.median],
            q3: [benchmarkData.cross.box_plot.q3],
            lowerfence: [benchmarkData.cross.box_plot.lowerfence],
            upperfence: [benchmarkData.cross.box_plot.upperfence]
          }
        ];

        const boxPlotLayout = {
          title: { text: 'Latency Variance (Box Plot)', font: { size: 20 } },
          yaxis: { 
            title: { text: 'Latency (ns) - LOG SCALE', font: { size: 16 } },
            type: 'log',
            tickfont: { size: 14 }
          },
          margin: { l: 80, b: 80, t: 60, r: 20 }
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
