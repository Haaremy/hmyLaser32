import Link from 'next/link';
import { getTranslations } from 'next-intl/server';

export default async function ResearchIndex() {
  const t = await getTranslations('research');
  return (
    <>
      <h1>{t('title')}</h1>
      <p>{t('intro')}</p>

      <section className="hmy-card">
        <div className="hmy-card__header">
          {t('paper_v1_title')}
        </div>
        <div className="hmy-card__body">
          <p style={{ color: 'var(--hmy-color-text-muted)', fontSize: 'var(--hmy-font-size-sm)', marginBottom: 'var(--hmy-spacing-2)' }}>
            {t('paper_v1_meta')}
          </p>
          <p style={{ margin: 0 }}>{t('paper_v1_abstract')}</p>
        </div>
        <div className="hmy-card__footer">
          <Link href="/research/paper-v1" className="hmy-btn hmy-btn--secondary">{t('read_online')}</Link>
          <a href="/papers/Lasertag_Paper_V1.pdf" download className="hmy-btn hmy-btn--primary">📄 {t('download_pdf')}</a>
        </div>
      </section>

      <section className="hmy-card">
        <div className="hmy-card__header" style={{ display: 'flex', alignItems: 'center', gap: '0.5rem' }}>
          {t('paper_v2_title')}
          <span className="hmy-lt-pill hmy-lt-pill--warning">{t('paper_v2_coming_soon')}</span>
        </div>
        <div className="hmy-card__body">
          <p style={{ margin: 0 }}>{t('paper_v2_abstract')}</p>
        </div>
        <div className="hmy-card__footer">
          <Link href="/research/paper-v2" className="hmy-btn hmy-btn--secondary">{t('read_online')}</Link>
          <span className="hmy-btn" aria-disabled="true">📄 {t('soon')}</span>
        </div>
      </section>
    </>
  );
}
