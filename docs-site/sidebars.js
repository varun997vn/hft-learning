// @ts-check

// This runs in Node.js - Don't use client-side code here (browser APIs, JSX...)

/**
 * Creating a sidebar enables you to:
 - create an ordered group of docs
 - render a sidebar for each doc of that group
 - provide next/previous navigation

 The sidebars can be generated from the filesystem, or explicitly defined here.

 Create as many sidebars as you want.

 @type {import('@docusaurus/plugin-content-docs').SidebarsConfig}
 */
const sidebars = {
  tutorialSidebar: [
    'simple-explanation',
    {
      type: 'category',
      label: 'NUMA SPSC Ring Buffer',
      items: [
        'introduction',
        'hardware-architecture',
        'software-primitives',
        'system-design',
        'benchmarking',
        'results-analysis'
      ],
    },
    {
      type: 'category',
      label: 'HFT Limit Order Book',
      items: [
        'lob-introduction',
        'big-picture',
        'lob-system-design',
        'lob-data-structures',
        'lob-cpp-features',
        'lob-benchmarking'
      ],
    },
    {
      type: 'category',
      label: 'End-to-End Pipeline',
      items: [
        'itch-pipeline-architecture',
        'ouch-protocol-gateway',
        'tcp-egress-networking',
        'pre-trade-risk-engine',
        'strategy-engine-framework',
        'execution-management-system',
        'async-lock-free-logger',
        'exchange-emulator',
        'tick-to-trade-metrics'
      ],
    },
    {
      type: 'category',
      label: 'System Architecture (UML)',
      items: [
        'uml/uml-Communication_diagram',
        'uml/uml-activity_diagram',
        'uml/uml-class_diagram',
        'uml/uml-component_diagram',
        'uml/uml-composite_structure_diagram',
        'uml/uml-deployment_diagram',
        'uml/uml-interaction_overview_diagram',
        'uml/uml-object_diagram',
        'uml/uml-package_diagram',
        'uml/uml-profile_diagram',
        'uml/uml-sequence_diagram',
        'uml/uml-statemachine_diagram',
        'uml/uml-timing_diagram',
        'uml/uml-usecase_diagram'
      ],
    },
  ],
};

export default sidebars;
