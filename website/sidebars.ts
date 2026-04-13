import type {SidebarsConfig} from '@docusaurus/plugin-content-docs';

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
          label: 'Editors',
          items: [
            'editors/bt-graph-editor',
            'editors/animation-editor',
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
          label: 'Entity Prefab Editor',
          items: [
            'user-guide/entity-prefab-editor/entity-prefab-overview',
            'user-guide/entity-prefab-editor/component-library',
            'user-guide/entity-prefab-editor/editor-guide',
            'user-guide/entity-prefab-editor/runtime-instantiation',
          ],
        },
        {
          type: 'category',
          label: 'Visual Scripting',
          items: [
            'user-guide/visual-scripting/visual-scripting-overview',
            'user-guide/visual-scripting/node-catalog',
            'user-guide/visual-scripting/task-execution',
            'user-guide/visual-scripting/best-practices',
          ],
        },
        {
          type: 'category',
          label: 'AI & Behavior Trees',
          items: [
            'user-guide/ai/bt-editor-overview',
            'user-guide/ai/bt-first-tree-tutorial',
            'user-guide/ai/bt-node-types',
            'user-guide/ai/bt-connection-rules',
            'user-guide/ai/bt-keyboard-shortcuts',
            'user-guide/ai/bt-debugger',
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
        {
          type: 'category',
          label: 'Animation System',
          items: [
            'user-guide/animation-system/animation-system-overview',
            'user-guide/animation-system/animation-system-quick-start',
            'user-guide/animation-system/animation-banks',
            'user-guide/animation-system/animation-graphs',
            'user-guide/animation-system/examples',
            'user-guide/animation-system/troubleshooting',
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
            'technical-reference/architecture/blackboard-architecture',
            'technical-reference/architecture/modules',
            'technical-reference/architecture/conceptual-schema',
            'technical-reference/architecture/dependency-schema',
            'technical-reference/architecture/implementation-summary',
          ],
        },
        {
          type: 'category',
          label: 'Canvas System',
          items: [
            'technical-reference/canvas-system/canvas-overview',
            'technical-reference/canvas-system/minimap-system',
            'technical-reference/canvas-system/coordinate-systems',
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
            'technical-reference/behavior-trees/bt-editor-architecture',
          ],
        },
        {
          type: 'category',
          label: 'Animation',
          items: [
            'technical-reference/animation/animation-system',
            'technical-reference/animation/animation-editor-architecture',
            'technical-reference/animation/api-reference',
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
        'contributing/coding-standards',
        'contributing/code-style',
        'contributing/testing-guide',
        'contributing/adding-components',
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
