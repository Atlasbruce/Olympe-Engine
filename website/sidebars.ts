import type {SidebarsConfig} from '@docusaurus/plugin-content-docs';

// This runs in Node.js - Don't use client-side code here (browser APIs, JSX...)

/**
 * Creating a sidebar enables you to:
 - create an ordered group of docs
 - render a sidebar for each doc of that group
 - provide next/previous navigation

 The sidebars can be generated from the filesystem, or explicitly defined here.

 Create as many sidebars as you want.
 */
const sidebars: SidebarsConfig = {
  gettingStartedSidebar: [
    'index',
    {
      type: 'category',
      label: 'Getting Started',
      collapsed: false,
      items: [
        'getting-started/prerequisites',
        'getting-started/installation',
        'getting-started/quick-start',
        'getting-started/project-structure',
      ],
    },
    {
      type: 'category',
      label: 'Tutorials',
      collapsed: false,
      items: [
        'tutorials/first-tiled-map',
        'tutorials/creating-entities',
        'tutorials/behavior-tree-basics',
      ],
    },
    {
      type: 'category',
      label: 'User Guide',
      collapsed: false,
      items: [
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
          label: 'Blueprint System',
          items: [
            'user-guide/blueprints/blueprints-overview',
            'user-guide/blueprints/creating-prefabs',
            'user-guide/blueprints/prefab-properties',
            'user-guide/blueprints/component-overrides',
          ],
        },
        {
          type: 'category',
          label: 'Input System',
          items: [
            'user-guide/input-system/input-configuration',
            'user-guide/input-system/input-architecture',
            'user-guide/input-system/input-user-guide',
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
          label: 'Architecture',
          items: [
            'technical-reference/architecture/ecs-overview',
            'technical-reference/architecture/ecs-components',
            'technical-reference/architecture/ecs-systems',
            'technical-reference/architecture/component-lifecycle',
            'technical-reference/architecture/modules',
            'technical-reference/architecture/implementation-summary',
          ],
        },
        {
          type: 'category',
          label: 'Behavior Trees',
          items: [
            'technical-reference/behavior-trees/behavior-trees-overview',
            'technical-reference/behavior-trees/bt-nodes',
            'technical-reference/behavior-trees/bt-custom-nodes',
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
      label: 'Contributing',
      collapsed: false,
      items: [
        'contributing/testing-guide',
        'contributing/adding-components',
        'contributing/code-style',
      ],
    },
  ],
  
  apiSidebar: [
    {
      type: 'category',
      label: 'API Reference',
      items: [
        'api-reference/api-index',
        'api-reference/api-introduction',
      ],
    },
  ],
};

export default sidebars;
