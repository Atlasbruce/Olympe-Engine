/**
 * Creating a sidebar enables you to:
 - create an ordered group of docs
 - render a sidebar for each doc of that group
 - provide next/previous navigation

 The sidebars can be generated from the filesystem, or explicitly defined here.

 Create as many sidebars as you want.
 */

// @ts-check

/** @type {import('@docusaurus/plugin-content-docs').SidebarsConfig} */
const sidebars = {
  // By default, Docusaurus generates a sidebar from the docs folder structure
  tutorialSidebar: [
    'index',
    {
      type: 'category',
      label: 'Getting Started',
      items: [
        'getting-started/introduction',
        'getting-started/installation',
        'getting-started/quick-start',
      ],
    },
    {
      type: 'category',
      label: 'User Guide',
      collapsed: false,
      items: [
        {
          type: 'category',
          label: 'Animation System',
          collapsed: false,
          items: [
            'user-guide/animation-system/animation-system-overview',
            'user-guide/animation-system/animation-system-quick-start',
            'user-guide/animation-system/animation-banks',
            'user-guide/animation-system/animation-graphs',
            'user-guide/animation-system/examples',
            'user-guide/animation-system/troubleshooting',
          ],
        },
        {
          type: 'category',
          label: 'Blueprints',
          items: [
            'user-guide/blueprints/blueprints-overview',
            'user-guide/blueprints/creating-prefabs',
            'user-guide/blueprints/prefab-properties',
            'user-guide/blueprints/component-overrides',
          ],
        },
        {
          type: 'category',
          label: 'Tiled Editor',
          items: [
            'user-guide/tiled-editor/tiled-introduction',
            'user-guide/tiled-editor/tiled-quick-start',
            'user-guide/tiled-editor/layer-properties',
            'user-guide/tiled-editor/object-properties',
            'user-guide/tiled-editor/best-practices',
          ],
        },
        {
          type: 'category',
          label: 'Input System',
          items: [
            'user-guide/input-system/input-architecture',
            'user-guide/input-system/input-user-guide',
            'user-guide/input-system/input-configuration',
          ],
        },
      ],
    },
    {
      type: 'category',
      label: 'Technical Reference',
      collapsed: false,
      items: [
        {
          type: 'category',
          label: 'Animation',
          items: [
            'technical-reference/animation/animation-system',
            'technical-reference/animation/api-reference',
          ],
        },
        {
          type: 'category',
          label: 'Architecture',
          items: [
            'technical-reference/architecture/implementation-summary',
            'technical-reference/architecture/ecs-overview',
            'technical-reference/architecture/ecs-components',
            'technical-reference/architecture/component-lifecycle',
            'technical-reference/architecture/ecs-systems',
            'technical-reference/architecture/modules',
          ],
        },
        {
          type: 'category',
          label: 'Behavior Trees',
          items: [
            'technical-reference/behavior-trees/behavior-trees-overview',
            'technical-reference/behavior-trees/nodes',
            'technical-reference/behavior-trees/custom-nodes',
            'technical-reference/behavior-trees/behavior-tree-dependency-loading',
          ],
        },
        {
          type: 'category',
          label: 'Navigation',
          items: [
            'technical-reference/navigation/navigation-system',
            'technical-reference/navigation/pathfinding',
            'technical-reference/navigation/navigation-mesh',
            'technical-reference/navigation/overlay-rendering',
          ],
        },
        {
          type: 'category',
          label: 'Security',
          items: [
            'technical-reference/security/security-summary',
          ],
        },
      ],
    },
    {
      type: 'category',
      label: 'API Reference',
      items: [
        'api-reference/api-index',
      ],
    },
    {
      type: 'category',
      label: 'Contributing',
      items: [
        'contributing/code-style',
      ],
    },
  ],
};

module.exports = sidebars;
