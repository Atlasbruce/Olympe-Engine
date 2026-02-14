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
            to="/getting-started/quick-start">
            Get Started →
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
                  Built on a powerful Entity-Component-System architecture for maximum flexibility
                  and performance. Create complex game logic with modular, reusable components.
                </p>
              </div>
            </div>
            <div className="col col--4">
              <div className="text--center padding-horiz--md">
                <h3>🗺️ Tiled Map Integration</h3>
                <p>
                  Seamlessly import maps from Tiled Map Editor with full support for layers,
                  tilesets, objects, and custom properties. Design your worlds visually.
                </p>
              </div>
            </div>
            <div className="col col--4">
              <div className="text--center padding-horiz--md">
                <h3>🤖 Behavior Trees & Navigation</h3>
                <p>
                  Advanced AI with behavior trees and A* pathfinding. Create intelligent NPCs
                  with complex decision-making and smooth navigation.
                </p>
              </div>
            </div>
          </div>
        </div>
      </main>
    </Layout>
  );
}
