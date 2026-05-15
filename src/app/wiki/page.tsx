import Link from 'next/link';
import { getTranslations } from 'next-intl/server';

export default async function WikiIndexPage() {
  const t = await getTranslations('wiki');
  return (
    <>
      <h1>{t('title')}</h1>
      <p>{t('intro')}</p>
      <div className="grid grid-2">
        <Link href="/wiki/hardware" className="feature"><h3>🔌 {t('hardware')}</h3></Link>
        <Link href="/wiki/architecture" className="feature"><h3>🧩 {t('architecture')}</h3></Link>
        <Link href="/wiki/build-guide" className="feature"><h3>🛠 {t('build_guide')}</h3></Link>
        <Link href="/wiki/components" className="feature"><h3>📦 {t('components')}</h3></Link>
      </div>
    </>
  );
}
