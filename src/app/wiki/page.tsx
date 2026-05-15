import Link from 'next/link';
import { getTranslations } from 'next-intl/server';

export default async function WikiIndexPage() {
  const t = await getTranslations('wiki');
  return (
    <>
      <h1>{t('title')}</h1>
      <p>{t('intro')}</p>

      <div className="card" style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', flexWrap: 'wrap', gap: '1rem' }}>
        <div>
          <h3 style={{ marginTop: 0 }}>📄 {t('paper_v1')}</h3>
          <p style={{ margin: 0 }}>Entwicklung eines lokalen Multiplayer Lasertag Games — Jeremy Becker, HS&nbsp;Anhalt</p>
        </div>
        <a className="btn btn-primary" href="/papers/Lasertag_Paper_V1.pdf" download>
          Download PDF (2.3&nbsp;MB)
        </a>
      </div>

      <div className="card" style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', flexWrap: 'wrap', gap: '1rem' }}>
        <div>
          <h3 style={{ marginTop: 0 }}>📄 {t('paper_v2')}</h3>
          <p style={{ margin: 0, color: 'var(--color-text-muted)' }}>{t('paper_v2_coming_soon')}</p>
        </div>
        <span className="btn" style={{ opacity: 0.5, cursor: 'not-allowed' }}>Coming soon</span>
      </div>

      <div className="grid grid-2">
        <Link href="/wiki/hardware" className="feature">
          <h3>🔌 {t('hardware')}</h3>
          <p>Funktionsanalyse Equipment · IR-Sender/Empfänger-Vergleich · Mikrocontroller</p>
        </Link>
        <Link href="/wiki/architecture" className="feature">
          <h3>🧩 {t('architecture')}</h3>
          <p>ESP-NOW Peer-to-Peer · Gossip-Protokoll · Punkteauswertung · Datenaustausch</p>
        </Link>
        <Link href="/wiki/build-guide" className="feature">
          <h3>🛠 {t('build_guide')}</h3>
          <p>Steckbrett → Lochplatine · Pin-Belegung · Brustmodul + Laserpistole</p>
        </Link>
        <Link href="/wiki/components" className="feature">
          <h3>📦 {t('components')}</h3>
          <p>Komponenten-Vergleich · Bauteilliste · Bezugsquellen</p>
        </Link>
      </div>
    </>
  );
}
