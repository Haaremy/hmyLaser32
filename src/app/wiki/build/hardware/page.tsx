const parts = [
  { name: 'ESP32 NodeMCU-32S', qty: '1 pro Spieler + 1 für Server', notes: 'Dual-Core 240 MHz, WLAN, ESP-NOW. Per USB programmierbar.' },
  { name: 'TSOP38238 IR-Empfänger', qty: '1 pro Spieler', notes: 'Bis 45 m Reichweite, 38 kHz, AGC2. Brustmodul.' },
  { name: 'KY-005 IR-Sender-Modul', qty: '1 pro Spieler', notes: '940 nm, ohne externen Transistor direkt vom ESP-GPIO steuerbar.' },
  { name: 'SSD1306 OLED 0.96″ I²C', qty: '1 pro Spieler', notes: '128 × 64 Pixel, zeigt HP / Score / Rank.' },
  { name: 'Taster (Auslöseknopf)', qty: '1 pro Spieler', notes: 'Einfacher Pull-Down auf GPIO 19.' },
  { name: '5 mm RGB-LED, gemeinsame Kathode', qty: '1 pro Spieler', notes: 'Status-LED am Brustmodul. 3 PWM-Pins (25/26/27).' },
  { name: 'Widerstand 330 Ω', qty: '3 pro RGB-LED', notes: 'Vorwiderstand pro Farbkanal.' },
  { name: 'Powerbank (5 V USB)', qty: '1 pro Spieler + 1 für Server', notes: 'Versorgt den ESP über USB. Keine Akku-Elektronik nötig.' },
  { name: 'Kupferkabel + Lötzinn', qty: 'nach Bedarf', notes: 'Lochplatinen-Verdrahtung.' },
  { name: 'Weste / Trägermaterial', qty: '1 pro Spieler', notes: 'Befestigung des Brustmoduls mit IR-Empfänger.' },
  { name: 'NFC-Karte (Mifare Classic 1K)', qty: 'optional, 1 pro Spieler', notes: 'Speichert username + nfcToken aus dem Web-Portal.' }
];

export default function BuildHardwarePage() {
  return (
    <article className="paper-prose">
      <h1>Finale Hardware-Liste</h1>
      <p>
        Das ist die Konfiguration, mit der der hmyLaser32-Prototyp in seiner
        ersten Version tatsächlich aufgebaut wurde — die Komponenten-Auswahl
        basiert auf den Versuchen aus <a href="/wiki/research/paper-v1">Paper V1</a>,
        ist hier aber kurz und ohne Vergleichstabellen.
      </p>

      <p className="alert alert-info" style={{ display: 'block' }}>
        <strong>Hinweise zur V1:</strong> Es gibt <em>noch kein Gehäuse</em>. Sender,
        Empfänger, Button, OLED und LED sind auf einer Lochplatine montiert und über
        Kabel verbunden. Versorgung erfolgt ausschließlich über USB-Powerbanks —
        keine LiPo-Lade­elektronik, keine 3,7&nbsp;V-Akkupack-Mathematik.
      </p>

      <h2>Pro Spieler-Set</h2>
      <table>
        <thead>
          <tr><th>Komponente</th><th>Menge</th><th>Hinweis</th></tr>
        </thead>
        <tbody>
          {parts.map((p) => (
            <tr key={p.name}>
              <td>{p.name}</td>
              <td>{p.qty}</td>
              <td>{p.notes}</td>
            </tr>
          ))}
        </tbody>
      </table>

      <h2>Zusätzlich für den Server</h2>
      <ul>
        <li>1 × ESP32 NodeMCU-32S</li>
        <li>1 × Powerbank</li>
        <li><em>Keine IR-Hardware, kein OLED, kein Button in V1.</em> Optional erweiterbar (siehe <a href="/wiki/build/firmware">Firmware-Seite</a>).</li>
      </ul>

      <h2>Warum diese Auswahl?</h2>
      <ul>
        <li><strong>TSOP38238</strong> — höchste gemessene Empfangsreichweite (bis 45 m, im Test 30 m im fensterlosen Flur). Details: <a href="/wiki/research/paper-v1#evaluation">Paper V1 §&nbsp;Evaluation</a>.</li>
        <li><strong>KY-005</strong> — funktioniert direkt am ESP-GPIO ohne zusätzlichen MOSFET. Wellenlänge passt zum TSOP.</li>
        <li><strong>NodeMCU-32S</strong> — günstig, Dual-Core, ESP-NOW ab Board-Version 3, ausreichend GPIO für alle Komponenten.</li>
        <li><strong>RGB-LED common cathode</strong> — gemeinsame GND, 3 PWM-Pins für Farbsteuerung. 330&nbsp;Ω passend für 5&nbsp;mm-LED bei 3,3&nbsp;V GPIO.</li>
        <li><strong>Powerbank</strong> — keine Akku-Lade­elektronik nötig; jede Standard-5&nbsp;V-USB-Powerbank reicht. ESP zieht typisch &lt; 200&nbsp;mA.</li>
      </ul>

      <h2>Bezugsquellen (Beispiele)</h2>
      <p style={{ color: 'var(--color-text-muted)' }}>
        Alle Komponenten sind als Sets oder einzeln bei AZ-Delivery, eckstein-shop, Reichelt,
        Conrad oder Amazon erhältlich. Das KY-Sensor-Set enthält typischerweise auch den
        KY-022 (Receiver) — diesen <em>nicht</em> verwenden, da der TSOP38238 deutlich
        mehr Reichweite hat.
      </p>
    </article>
  );
}
