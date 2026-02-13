// @ts-check
// Note: type annotations allow type checking and IDEs autocompletion

const {themes} = require('prism-react-renderer');
const lightTheme = themes.github;
const darkTheme = themes.dracula;

/** @type {import('@docusaurus/types').Config} */
const config = {
  title: 'Olympe Engine',
  tagline: '2D Game Engine with ECS Architecture',
  favicon: 'img/logo.png',

  // Set the production url of your site here
  url: 'https://atlasbruce.github.io',
  // Set the /<baseUrl>/ pathname under which your site is served
  baseUrl: '/Olympe-Engine/',

  // GitHub pages deployment config.
  organizationName: 'Atlasbruce',
  projectName: 'Olympe-Engine',

  onBrokenLinks: 'warn',
  onBrokenMarkdownLinks: 'warn',

  // Even if you don't use internalization, you can use this field to set useful
  // metadata like html lang. For example, if your site is Chinese, you may want
  // to replace "en" with "zh-Hans".
  i18n: {
    defaultLocale: 'en',
    locales: ['en'],
  },

  presets: [
    [
      'classic',
      /** @type {import('@docusaurus/preset-classic').Options} */
      ({
        docs: {
          sidebarPath: require.resolve('./sidebars.js'),
          editUrl: 'https://github.com/Atlasbruce/Olympe-Engine/tree/master/website/',
        },
        blog: {
          showReadingTime: true,
          editUrl: 'https://github.com/Atlasbruce/Olympe-Engine/tree/master/website/',
        },
        theme: {
          customCss: require.resolve('./src/css/custom.css'),
        },
      }),
    ],
  ],

  themeConfig:
    /** @type {import('@docusaurus/preset-classic').ThemeConfig} */
    ({
      image: 'img/logo.png',
      navbar: {
        title: 'Olympe Engine',
        logo: {
          alt: 'Olympe Engine Logo',
          src: 'img/logo.png',
        },
        items: [
          {
            type: 'doc',
            docId: 'index',
            position: 'left',
            label: 'Docs',
          },
          {
            to: '/api/html/index.html',
            label: 'API Reference',
            position: 'left',
          },
          {
            to: '/blog',
            label: 'Blog',
            position: 'left'
          },
          {
            href: 'https://github.com/Atlasbruce/Olympe-Engine',
            label: 'GitHub',
            position: 'right',
          },
        ],
      },
      footer: {
        style: 'dark',
        links: [
          {
            title: 'Docs',
            items: [
              {
                label: 'Documentation',
                to: '/docs',
              },
              {
                label: 'API Reference',
                to: '/api/html/index.html',
              },
            ],
          },
          {
            title: 'Community',
            items: [
              {
                label: 'GitHub',
                href: 'https://github.com/Atlasbruce/Olympe-Engine',
              },
            ],
          },
          {
            title: 'More',
            items: [
              {
                label: 'Blog',
                to: '/blog',
              },
            ],
          },
        ],
        copyright: `Copyright © ${new Date().getFullYear()} Olympe Engine. Built with Docusaurus.`,
      },
      prism: {
        theme: lightTheme,
        darkTheme: darkTheme,
        additionalLanguages: ['cpp', 'cmake', 'json', 'bash'],
      },
    }),
};

module.exports = config;
