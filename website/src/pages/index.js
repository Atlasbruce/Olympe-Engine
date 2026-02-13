import clsx from 'clsx';
import Link from '@docusaurus/Link';
import useDocusaurusContext from '@docusaurus/useDocusaurusContext';
import Layout from '@theme/Layout';
import Heading from '@theme/Heading';

import styles from './index.module.css';

function HomepageHeader() {
  const {siteConfig} = useDocusaurusContext();
  return (
    <header className={clsx('hero hero--primary', styles.heroBanner)}>
      <div className="container">
        <Heading as="h1" className="hero__title">
          {siteConfig.title}
        </Heading>
        <p className="hero__subtitle">{siteConfig.tagline}</p>
        <div className={styles.buttons}>
          <Link
            className="button button--secondary button--lg"
            to="/docs">
            Get Started - 5min ⏱️
          </Link>
        </div>
      </div>
    </header>
  );
}

export default function Home() {
  const {siteConfig} = useDocusaurusContext();
  return (
    <Layout
      title={`Hello from ${siteConfig.title}`}
      description="2D Game Engine with ECS Architecture">
      <HomepageHeader />
      <main>
        <div className="container margin-vert--xl">
          <div className="row">
            <div className="col col--4">
              <div className="text--center padding-horiz--md">
                <h3>🎮 ECS Architecture</h3>
                <p>
                  Built on Entity Component System for performance and flexibility
                </p>
              </div>
            </div>
            <div className="col col--4">
              <div className="text--center padding-horiz--md">
                <h3>🤖 Advanced AI</h3>
                <p>
                  Behavior trees, pathfinding, and navigation systems included
                </p>
              </div>
            </div>
            <div className="col col--4">
              <div className="text--center padding-horiz--md">
                <h3>🗺️ Tiled Integration</h3>
                <p>
                  Full support for Tiled maps with multiple projections
                </p>
              </div>
            </div>
          </div>
        </div>
      </main>
    </Layout>
  );
}
