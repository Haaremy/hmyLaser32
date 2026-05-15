import { getTranslations } from 'next-intl/server';

export default async function HardwarePage() {
  const t = await getTranslations('wiki');
  return (
    <article className="paper-prose">
      <h1>{t('hardware')}</h1>
      <p style={{ color: 'var(--color-text-muted)', fontSize: '0.9em' }}>
        Aus: <em>Entwicklung eines lokalen Multiplayer Lasertag Games</em>, Jeremy Becker (HS&nbsp;Anhalt).
        Volltext im <a href="/papers/Lasertag_Paper_V1.pdf">{t('paper_v1')}</a>.
      </p>

      <h2>Funktionsanalyse Equipment</h2>

      <h3>Weste</h3>
      <p>
        Die Lasertagweste enthält neben den Infrarotsensoren den Spieler-Controller (SpiCo). An der
        Weste sind Infrarotsensoren angebracht, die das von der Laserpistole gesendete Infrarotsignal
        empfangen. Daraufhin wird im SpiCo die Punkteberechnung ausgeführt und die Daten an einen
        zentralen Computer zur Punkteauswertung aller Spieler übermittelt. Die Kommunikation zwischen
        Weste und zentralem Server erfolgt in kommerziellen Systemen drahtlos, typischerweise über
        Funkprotokolle oder WLAN. In den meisten Fällen sind Sensoren an Schulter, Bauch und Rücken
        angebracht, seltener auch an Stirn und Arm. Mehr Sensoren erhöhen die Trefferwahrscheinlichkeit
        und tragen zur Fairness des Spiels bei.
      </p>

      <h3>Laserpistole</h3>
      <p>
        Die Laserpistole sendet einen für den Spieler sichtbaren Laserpointerstrahl. Beim Drücken des
        Abzugs wird ein Infrarotsignal ausgelöst, das die Spieler-ID enthält. Trifft das Signal die
        Empfangseinheit eines Gegners, registriert dessen Mikrocontroller den Treffer.
      </p>

      <h2>Komponentenauswahl</h2>

      <h3>Infrarotsender</h3>
      <p>Sechs Kandidaten wurden evaluiert. Grundlegendes Kriterium: Reichweite ≥&nbsp;5&nbsp;m und Kompatibilität mit dem ESP32.</p>

      <table>
        <thead>
          <tr><th>Bauteil</th><th>λ [nm]</th><th>Winkel (ges. / ±)</th><th>Reichweite</th></tr>
        </thead>
        <tbody>
          <tr><td>Grove IR Emitter</td><td className="num">940</td><td>34° / ±17°</td><td className="num">10&nbsp;m</td></tr>
          <tr><td>IR Fernbedienung</td><td className="num">≈940</td><td>≈60° / ±30°</td><td className="num">5–10&nbsp;m</td></tr>
          <tr><td>Kemo S081</td><td className="num">870/925</td><td>—</td><td>—</td></tr>
          <tr><td>TSUS 5202</td><td className="num">950</td><td>30° / ±15°</td><td>—</td></tr>
          <tr><td><strong>KY-005</strong></td><td className="num">940</td><td>40° / ±20°</td><td>—</td></tr>
          <tr><td>Gravity DFR0095</td><td className="num">940</td><td>—</td><td>—</td></tr>
          <tr><td>YIXISI IR Emitter</td><td className="num">850</td><td>45°</td><td className="num">8&nbsp;m</td></tr>
          <tr><td>YIXISI IR Emitter</td><td className="num">940</td><td>45°</td><td>—</td></tr>
        </tbody>
      </table>
      <p style={{ fontSize: '0.85em', color: 'var(--color-text-muted)' }}>λ: Spitzenwellenlänge. — = keine Herstellerangabe.</p>

      <h3>Infrarotempfänger</h3>
      <p>Vier Empfänger evaluiert. Alle arbeiten bei 38&nbsp;kHz Trägerfrequenz mit Spitzenwellenlänge 940–950&nbsp;nm.</p>
      <table>
        <thead>
          <tr><th>Bauteil</th><th>Winkel (ges. / ±)</th><th>Reichweite</th></tr>
        </thead>
        <tbody>
          <tr><td>Grove IR Receiver</td><td>90° / ±45°</td><td className="num">10&nbsp;m</td></tr>
          <tr><td><strong>TSOP38238</strong></td><td>90° / ±45°</td><td className="num">45&nbsp;m</td></tr>
          <tr><td>TSOP4838</td><td>90° / ±45°</td><td className="num">35&nbsp;m</td></tr>
          <tr><td><strong>KY-022</strong></td><td>90° / ±45°</td><td className="num">18&nbsp;m</td></tr>
        </tbody>
      </table>
      <p style={{ fontSize: '0.85em', color: 'var(--color-text-muted)' }}>
        TSOP38238 (AGC2): Standardanwendungen; TSOP4838 (AGC1): Lange Bursts (RC-5, RC-6).
      </p>

      <div className="grid grid-3" style={{ margin: '2rem 0' }}>
        <figure>
          <img src="/papers/v1/KY-005.png" alt="KY-005 IR-Sender" />
          <figcaption>{t('figure')} 1: KY-005 IR-Sender</figcaption>
        </figure>
        <figure>
          <img src="/papers/v1/KY-022.png" alt="KY-022 IR-Empfänger" />
          <figcaption>{t('figure')} 2: KY-022 IR-Empfänger</figcaption>
        </figure>
        <figure>
          <img src="/papers/v1/TSOP38238.jpg" alt="TSOP38238 IR-Empfängermodul" />
          <figcaption>{t('figure')} 3: TSOP38238 IR-Empfängermodul</figcaption>
        </figure>
      </div>

      <h3>Controller</h3>
      <p>Für das Lasertag-System ist insbesondere native WLAN-Unterstützung entscheidend.</p>
      <table>
        <thead>
          <tr><th>Gerät</th><th>V<sub>in</sub> / V<sub>out</sub> [V]</th><th>Protokolle</th></tr>
        </thead>
        <tbody>
          <tr><td>Arduino Uno / Mega / Nano</td><td className="num">7–12 / 5</td><td>UART, I²C, SPI</td></tr>
          <tr><td>ESP8266</td><td className="num">5 / 3,3</td><td>WLAN, UART, I²C, SPI</td></tr>
          <tr><td><strong>ESP32</strong></td><td className="num">5 / 3,3</td><td><strong>WLAN, BT, UART, I²C, SPI</strong></td></tr>
          <tr><td>RPi Zero W</td><td className="num">5 / 3,3</td><td>WLAN, BT, UART, I²C, SPI</td></tr>
          <tr><td>RPi 3B</td><td className="num">5 / 3,3</td><td>WLAN, ETH, BT, SPI</td></tr>
        </tbody>
      </table>

      <figure>
        <img src="/papers/v1/ESP32-S3_on_paper.jpg" alt="ESP32-S3 Entwicklungsboard" style={{ maxWidth: '460px' }} />
        <figcaption>{t('figure')} 4: ESP32-S3 Entwicklungsboard</figcaption>
      </figure>

      <h3>Zusammenfassung der Hardwarewahl</h3>
      <p>
        Für den vorliegenden Aufbau wird das <strong>KY-Infrarot-Set</strong> gewählt. Das KY-005
        Sendemodul benötigt keinen zusätzlichen Transistor; das KY-022 weist mit 18&nbsp;m gegenüber dem
        Grove Receiver deutlich höhere Empfangsreichweite auf. Ergänzend wird der TSOP38238 in einem
        Parallelversuch getestet (höchste Empfangsreichweite). Der ESP32 wird gegenüber dem ESP8266
        bevorzugt — Dual-Core-CPU mit 240&nbsp;MHz erlaubt die Trennung von IR-Signalverarbeitung und
        WLAN-Kommunikation auf je einen Kern.
      </p>

      <h3>Display</h3>
      <figure>
        <img src="/papers/v1/oled.png" alt="OLED Display I²C 128 × 64 Pixel" style={{ maxWidth: '320px' }} />
        <figcaption>{t('figure')} 5: OLED-Display I²C, 128&nbsp;×&nbsp;64&nbsp;Pixel</figcaption>
      </figure>
      <p>
        Zur Anzeige von Spielerstatistiken (HP, Munition, Score) dient ein SSD1306-basiertes OLED mit
        128&nbsp;×&nbsp;64&nbsp;Pixel über I²C.
      </p>
    </article>
  );
}
