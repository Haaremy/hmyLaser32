export default function PaperV1Page() {
  return (
    <article className="paper-prose">
      <header>
        <h1>Paper V1 — Entwicklung eines lokalen Multiplayer Lasertag Games</h1>
        <p style={{ color: 'var(--color-text-muted)', fontSize: '0.9em' }}>
          Jeremy Becker · Fachbereich Informatik und Sprachen · Hochschule Anhalt, Köthen
        </p>
        <p>
          <a className="btn btn-primary" href="/papers/Lasertag_Paper_V1.pdf" download>
            📄 Volltext herunterladen (PDF, 2.3 MB)
          </a>
        </p>
      </header>

      <h2>Kurzfassung</h2>
      <p>
        In dieser Arbeit wird mit verschiedenen Sensoren und Mikrocontrollern ein
        Prototyp-Lasertag-Spiel für mindestens zwei Spieler entwickelt. Ziel ist eine
        Spielerfahrung ähnlich kommerziellem Lasertag mit kostengünstigen Komponenten.
        Fokus: das System aus Laserpistole und Weste sowie der Punkteabgleich zwischen
        Spielern.
      </p>

      <h2>Funktionsanalyse Equipment</h2>
      <h3>Weste</h3>
      <p>
        Die Lasertagweste enthält neben Infrarotsensoren den Spieler-Controller (SpiCo).
        Sensoren empfangen IR-Signale der Pistole, der SpiCo führt die Punkteberechnung
        aus und übermittelt Daten an einen zentralen Computer. In kommerziellen Systemen
        erfolgt die Kommunikation drahtlos, typischerweise über WLAN. Sensoren sitzen
        meist an Schulter, Bauch und Rücken.
      </p>

      <h3>Laserpistole</h3>
      <p>
        Die Laserpistole sendet einen sichtbaren Laserpointerstrahl. Beim Knopfdruck
        wird ein IR-Signal mit Spieler-ID ausgelöst.
      </p>

      <h2>Komponentenauswahl</h2>
      <h3>Infrarotsender — 6 Kandidaten evaluiert</h3>
      <table>
        <thead>
          <tr><th>Bauteil</th><th>λ [nm]</th><th>Winkel (ges. / ±)</th><th>Reichweite</th></tr>
        </thead>
        <tbody>
          <tr><td>Grove IR Emitter</td><td className="num">940</td><td>34° / ±17°</td><td className="num">10 m</td></tr>
          <tr><td>IR Fernbedienung</td><td className="num">≈940</td><td>≈60° / ±30°</td><td className="num">5–10 m</td></tr>
          <tr><td>Kemo S081</td><td className="num">870/925</td><td>—</td><td>—</td></tr>
          <tr><td>TSUS 5202</td><td className="num">950</td><td>30° / ±15°</td><td>—</td></tr>
          <tr><td><strong>KY-005</strong></td><td className="num">940</td><td>40° / ±20°</td><td>—</td></tr>
          <tr><td>Gravity DFR0095</td><td className="num">940</td><td>—</td><td>—</td></tr>
          <tr><td>YIXISI IR Emitter</td><td className="num">850</td><td>45°</td><td className="num">8 m</td></tr>
          <tr><td>YIXISI IR Emitter</td><td className="num">940</td><td>45°</td><td>—</td></tr>
        </tbody>
      </table>

      <h3>Infrarotempfänger — 4 Kandidaten</h3>
      <table>
        <thead>
          <tr><th>Bauteil</th><th>Winkel (ges. / ±)</th><th>Reichweite</th></tr>
        </thead>
        <tbody>
          <tr><td>Grove IR Receiver</td><td>90° / ±45°</td><td className="num">10 m</td></tr>
          <tr><td><strong>TSOP38238</strong></td><td>90° / ±45°</td><td className="num">45 m</td></tr>
          <tr><td>TSOP4838</td><td>90° / ±45°</td><td className="num">35 m</td></tr>
          <tr><td>KY-022</td><td>90° / ±45°</td><td className="num">18 m</td></tr>
        </tbody>
      </table>

      <h3>Controller</h3>
      <table>
        <thead>
          <tr><th>Gerät</th><th>V<sub>in</sub> / V<sub>out</sub> [V]</th><th>Protokolle</th></tr>
        </thead>
        <tbody>
          <tr><td>Arduino Uno / Mega / Nano</td><td className="num">7–12 / 5</td><td>UART, I²C, SPI</td></tr>
          <tr><td>ESP8266</td><td className="num">5 / 3,3</td><td>WLAN, UART, I²C, SPI</td></tr>
          <tr><td><strong>ESP32</strong></td><td className="num">5 / 3,3</td><td><strong>WLAN, BT, UART, I²C, SPI</strong></td></tr>
          <tr><td>RPi Zero W / 3B</td><td className="num">5 / 3,3</td><td>WLAN, BT, ETH</td></tr>
        </tbody>
      </table>

      <div className="grid grid-3" style={{ margin: '2rem 0' }}>
        <figure>
          <img src="/papers/v1/KY-005.png" alt="KY-005 IR-Sender" />
          <figcaption>KY-005 IR-Sender</figcaption>
        </figure>
        <figure>
          <img src="/papers/v1/TSOP38238.jpg" alt="TSOP38238 IR-Empfänger" />
          <figcaption>TSOP38238 IR-Empfänger</figcaption>
        </figure>
        <figure>
          <img src="/papers/v1/ESP32-S3_on_paper.jpg" alt="ESP32 Entwicklungsboard" />
          <figcaption>ESP32-Entwicklungsboard</figcaption>
        </figure>
      </div>

      <h2 id="datatransfer">Datenaustausch — ESP-NOW + Gossip</h2>
      <p>
        Der ESP32 bietet ab Boardversion 3 das <strong>ESP-NOW-Protokoll</strong> —
        Peer-to-Peer-Kommunikation im 2,4-GHz-WLAN-Band ohne Router. Auf dieser
        Basis kommt ein <strong>Gossip-ähnliches Verfahren</strong> mit Anti-Entropy
        zum Einsatz: jeder Empfänger eines Treffers passt seine Daten an, broadcastet
        das aktuelle Punkte-Array mit Zeitstempeln, andere Geräte gleichen ab.
      </p>

      <h2 id="evaluation">Evaluation — Reichweitenmessungen</h2>
      <p>
        Drei ESP32-Testeinheiten in vier Szenarien: Innenbereich (Fachbereich INS, Halle),
        Außenbereich mit Sonne, Außen bei bewölkt, sowie separat ESP-NOW-Reichweiten.
      </p>

      <h3>Szenario 1 — Innenbereich</h3>
      <table>
        <thead>
          <tr><th>Meter</th><th>KY-KY</th><th>YI9-KY</th><th>KY-T</th><th>YI9-T</th><th>YI8-KY</th><th>YI8-T</th></tr>
        </thead>
        <tbody>
          <tr><td>1</td><td>10/10</td><td>5/5</td><td>5/5</td><td>—</td><td>5/5</td><td>5/5</td></tr>
          <tr><td>2</td><td>12/5</td><td>10/0</td><td>5/5</td><td>5/5</td><td>10/0</td><td>10/2</td></tr>
          <tr><td>4</td><td>10/0</td><td>—</td><td>5/5</td><td>10/3</td><td>—</td><td>10/0</td></tr>
          <tr><td>12</td><td>—</td><td>—</td><td>10/7</td><td>—</td><td>—</td><td>—</td></tr>
          <tr><td>20</td><td>—</td><td>—</td><td>20/4</td><td>—</td><td>—</td><td>—</td></tr>
          <tr><td>30</td><td>—</td><td>—</td><td>10/1</td><td>—</td><td>—</td><td>—</td></tr>
        </tbody>
      </table>
      <p style={{ fontSize: '0.85em', color: 'var(--color-text-muted)' }}>
        Format: Schüsse/Treffer. KY = KY-005 (Sender) bzw. KY-022 (Empfänger);
        T = TSOP38238; YI8/YI9 = YIXIS-Sender 850 nm / 940 nm.
      </p>

      <h3>Szenario 4 — ESP-NOW-Reichweite</h3>
      <table>
        <thead>
          <tr><th>Meter</th><th>Flure</th><th>Räume</th><th>Outdoor</th></tr>
        </thead>
        <tbody>
          <tr><td>5–15</td><td>ja</td><td>ja</td><td>ja</td></tr>
          <tr><td>20–50</td><td>ja</td><td>nein</td><td>ja</td></tr>
          <tr><td>55–65</td><td>n.b.</td><td>nein</td><td>ja</td></tr>
          <tr><td>65+</td><td>n.b.</td><td>nein</td><td>nein</td></tr>
        </tbody>
      </table>

      <h2>Fazit</h2>
      <p>
        Höchste Reichweite IR: <strong>~30 m</strong> in fensterlosem Kellerflur ohne
        natürliches Licht (KY-005 → TSOP38238). Direkte Sonneneinstrahlung reduziert die
        Reichweite drastisch auf 2 m. ESP-NOW kommuniziert outdoor zuverlässig bis 65 m.
        Das Konzept ist mit Standardkomponenten realisierbar — der Prototyp ersetzt
        keine kommerziellen Anlagen, demonstriert aber Machbarkeit.
      </p>

      <p style={{ marginTop: '2rem' }}>
        <a className="btn" href="/papers/Lasertag_Paper_V1.pdf" download>
          📄 Volltext mit allen Tabellen und Quellen herunterladen
        </a>
      </p>
    </article>
  );
}
