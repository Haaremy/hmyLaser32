import { getTranslations } from 'next-intl/server';

const components = [
  { name: 'ESP32-DevKit-C (ESP32-WROOM-32)', qty: '1 pro Gerät', notes: 'Dual-Core 240 MHz, WLAN + BT, ESP-NOW ab Board-Version 3' },
  { name: 'KY-005 IR-Sender-Modul', qty: '1 pro Client', notes: '940 nm, 40°/±20°, kein zusätzlicher Transistor nötig' },
  { name: 'KY-022 IR-Empfänger-Modul', qty: '1 pro Client', notes: '90°/±45°, 18 m Reichweite, VS1838B-basiert' },
  { name: 'TSOP38238 (alternativ)', qty: 'optional', notes: 'Bis zu 45 m Empfangsreichweite, AGC2' },
  { name: 'OLED-Display SSD1306 0.96″ I²C', qty: '1 pro Client', notes: '128 × 64 Pixel, zeigt HP/Munition/Score' },
  { name: 'Auslöseknopf (Taster)', qty: '1 pro Client', notes: 'Pull-Down auf GPIO 19' },
  { name: 'Powerbank / LiPo 1S 1000 mAh', qty: '1 pro Client', notes: 'Energieversorgung, ~6 h Spielzeit' },
  { name: 'Lochplatine / Breadboard', qty: '1 pro Client', notes: 'Prototyp auf Steckbrett, final auf Lochplatine' },
  { name: 'Kupferkabel + Lötzinn', qty: 'nach Bedarf', notes: 'Signal- und Versorgungsleitungen' },
  { name: 'Weste / Trägermaterial', qty: '1 pro Spieler', notes: 'Befestigung Brustmodul mit Empfänger' },
  { name: 'NFC-Karte (Mifare Classic 1K)', qty: '1 pro Spieler', notes: 'Speichert username + nfcToken aus dem Portal' }
];

export default async function ComponentsPage() {
  const t = await getTranslations('wiki');
  return (
    <article className="paper-prose">
      <h1>{t('components')}</h1>
      <p style={{ color: 'var(--color-text-muted)', fontSize: '0.9em' }}>
        Aus: <em>Entwicklung eines lokalen Multiplayer Lasertag Games</em>, Jeremy Becker.
        Volltext im <a href="/papers/Lasertag_Paper_V1.pdf">{t('paper_v1')}</a>.
      </p>

      <h2>Bauteilliste</h2>
      <table>
        <thead>
          <tr><th>Komponente</th><th>Menge</th><th>Hinweis</th></tr>
        </thead>
        <tbody>
          {components.map((c) => (
            <tr key={c.name}>
              <td>{c.name}</td>
              <td>{c.qty}</td>
              <td>{c.notes}</td>
            </tr>
          ))}
        </tbody>
      </table>

      <h2>Begründung der Auswahl</h2>
      <p>
        Hauptgrund für das KY-Infrarot-Set ist das Receiver-Modul KY-022 — es weist gegenüber dem
        Grove Receiver eine deutlich höhere Empfangsreichweite auf (18&nbsp;m vs. 10&nbsp;m). Im Set
        besteht eine Kompatibilitätsgarantie zwischen Empfänger und Sender. Der TSOP38238 wird
        ergänzend evaluiert, da er die höchste Empfangsreichweite aller getesteten Module bietet
        (bis 45&nbsp;m). Der limitierende Faktor ist der Sender — dessen Strahlungsintensität,
        Wellenlänge und Abstrahlwinkel bestimmen die erzielbare Reichweite maßgeblich.
      </p>
      <p>
        Der ESP32 wird gegenüber dem ESP8266 bevorzugt, weil er über eine Dual-Core-CPU mit
        240&nbsp;MHz verfügt — IR-Signalverarbeitung und WLAN-Kommunikation lassen sich auf zwei
        Kerne verteilen, was einen stabileren Betrieb gewährleistet.
      </p>
    </article>
  );
}
