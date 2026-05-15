import { getTranslations } from 'next-intl/server';

const components = [
  { name: 'ESP32-DevKit-C', qty: '1 pro Gerät', notes: 'Auch ESP32-WROOM-32 baugleich' },
  { name: 'IR-LED 940 nm (TSAL6200)', qty: '1 pro Client', notes: 'Optisch fokussiert für Reichweite' },
  { name: 'TSOP4838 IR-Empfänger', qty: '1 pro Client', notes: '38 kHz Trägerfrequenz' },
  { name: 'MOSFET IRLZ44 (Logic-Level)', qty: '1 pro Client', notes: 'Schaltet IR-LED-Strom' },
  { name: 'SSD1306 OLED 0.96″ I²C', qty: '1 pro Client', notes: 'Anzeige HP / Munition / Score' },
  { name: 'LiPo 1S 1000 mAh', qty: '1 pro Client', notes: 'Akku, ca. 6h Laufzeit' },
  { name: 'TP4056 Charge-Modul', qty: '1 pro Client', notes: 'Ladeelektronik mit Schutzschaltung' },
  { name: 'MT3608 Step-Up Wandler', qty: '1 pro Client', notes: '3.7 V → 5 V für OLED' },
  { name: '3D-gedrucktes Gehäuse', qty: '1 pro Client', notes: 'STL-Dateien im Repo' },
  { name: 'NFC-Karten (Mifare Classic 1K)', qty: '1 pro Spieler', notes: 'Speichert username + nfcToken' }
];

export default async function ComponentsPage() {
  const t = await getTranslations('wiki');
  return (
    <>
      <h1>{t('components')}</h1>
      <table>
        <thead>
          <tr><th>Component</th><th>Qty</th><th>Notes</th></tr>
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
    </>
  );
}
