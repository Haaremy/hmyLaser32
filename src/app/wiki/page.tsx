import Link from 'next/link';
import { getTranslations } from 'next-intl/server';

export default async function WikiIndexPage() {
  const t = await getTranslations('wiki');
  return (
    <>
      <h1>{t('title')}</h1>
      <p>{t('intro')}</p>

      <h2 style={{ marginTop: '2rem' }}>🛠 Bauanleitung — was tatsächlich gebaut wird</h2>
      <p style={{ marginTop: 0, color: 'var(--color-text-secondary)' }}>
        Schritt-für-Schritt für den finalen Prototyp: Hardware-Liste, Verdrahtung,
        Firmware-Flash und erste Inbetriebnahme.
      </p>
      <div className="grid grid-2">
        <Link href="/wiki/build/hardware" className="feature">
          <h3>🔌 Hardware</h3>
          <p>Finale Komponentenliste — was im aktuellen Build wirklich verbaut ist.</p>
        </Link>
        <Link href="/wiki/build/wiring" className="feature">
          <h3>🪛 Verdrahtung</h3>
          <p>Pin-Belegung, Steckbrett-Schema, Lötanleitung für Pistole + Brustmodul.</p>
        </Link>
        <Link href="/wiki/build/firmware" className="feature">
          <h3>💾 Firmware</h3>
          <p>Arduino-Setup, Module-Übersicht, Konfiguration pro Spieler, Flash-Anleitung.</p>
        </Link>
        <Link href="/wiki/build/run" className="feature">
          <h3>🎯 Erstes Spiel</h3>
          <p>ESP-Server koppeln, Lobby-Timer, Match starten, Live-Status verfolgen.</p>
        </Link>
      </div>

      <h2 style={{ marginTop: '3rem' }}>📑 Wissenschaftliche Arbeit — was untersucht wurde</h2>
      <p style={{ marginTop: 0, color: 'var(--color-text-secondary)' }}>
        Die Konzeption, der Komponenten-Vergleich und die Messreihen, die zur finalen
        Hardware-Wahl geführt haben. Lehrmaterial — kein 1:1-Build-Manual.
      </p>
      <div className="grid grid-2">
        <Link href="/wiki/research/paper-v1" className="feature">
          <h3>📄 Paper V1 — Lokales Multiplayer Lasertag</h3>
          <p>
            Funktionsanalyse, Komponenten-Vergleich (6 IR-Sender / 4 Empfänger / 7 Controller),
            ESP-NOW + Gossip, Reichweitenmessungen indoor/outdoor.
          </p>
        </Link>
        <Link href="/wiki/research/paper-v2" className="feature">
          <h3>📄 Paper V2 — Erweiterung</h3>
          <p style={{ color: 'var(--color-text-muted)' }}>{t('paper_v2_coming_soon')}</p>
        </Link>
      </div>
    </>
  );
}
