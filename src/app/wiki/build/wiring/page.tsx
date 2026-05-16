export default function BuildWiringPage() {
  return (
    <article className="paper-prose">
      <h1>Verdrahtung — Pin-Belegung & Aufbau</h1>
      <p>
        Wer das System nachbauen will, kann zunächst auf einem Steckbrett testen
        und später auf eine Lochplatine löten. Die Pin-Belegung ist identisch
        und in der Firmware <code>Config.h</code> hinterlegt.
      </p>

      <h2>Pin-Belegung Client (Spieler-ESP)</h2>
      <table>
        <thead>
          <tr><th>GPIO</th><th>Funktion</th><th>Komponente</th></tr>
        </thead>
        <tbody>
          <tr><td className="num">4</td><td>IR-Sender Output</td><td>KY-005 Signal</td></tr>
          <tr><td className="num">14</td><td>IR-Receiver Input</td><td>TSOP38238 Signal</td></tr>
          <tr><td className="num">19</td><td>Button (Pull-Up)</td><td>Auslöseknopf</td></tr>
          <tr><td className="num">21</td><td>I²C SDA</td><td>OLED-Display</td></tr>
          <tr><td className="num">22</td><td>I²C SCL</td><td>OLED-Display</td></tr>
          <tr><td className="num">25</td><td>PWM Rot</td><td>RGB-LED Rot (über 330 Ω)</td></tr>
          <tr><td className="num">26</td><td>PWM Grün</td><td>RGB-LED Grün (über 330 Ω)</td></tr>
          <tr><td className="num">27</td><td>PWM Blau</td><td>RGB-LED Blau (über 330 Ω)</td></tr>
          <tr><td>3V3</td><td>Versorgung</td><td>OLED VCC, TSOP VCC, KY-005 VCC</td></tr>
          <tr><td>GND</td><td>Masse</td><td>Alle Komponenten + gem. Kathode der RGB-LED</td></tr>
        </tbody>
      </table>

      <p className="alert alert-info" style={{ display: 'block' }}>
        <strong>RGB-LED common cathode:</strong> Die längste Pin ist GND. Die drei
        Farb-Pins gehen über jeweils 330&nbsp;Ω an GPIO 25/26/27. In <code>Config.h</code> ist
        <code> RGB_COMMON_ANODE&nbsp;=&nbsp;false</code> gesetzt — wenn du
        common-anode-LEDs verwendest: auf <code>true</code> stellen.
      </p>

      <h2>Aufbau-Schritte</h2>
      <ol>
        <li>
          <strong>Steckbrett-Prototyp.</strong> Erst alle Komponenten ohne Löten zusammenstecken
          und testen, ob Button, IR-Send, IR-Receive, OLED und LED funktionieren.
        </li>
        <li>
          <strong>Lochplatine.</strong> Sobald der Prototyp läuft, alles auf eine Lochplatine
          übertragen — sauber löten, sonst werden Signal-Glitches auf den I²C-Leitungen
          schwer zu debuggen.
        </li>
        <li>
          <strong>Brustmodul.</strong> ESP32 + TSOP38238 + RGB-LED + Powerbank-USB-Anschluss
          zusammen. Der TSOP zeigt nach vorne (Treffer-Richtung).
        </li>
        <li>
          <strong>Pistole.</strong> KY-005 + Button + OLED + Kabel zur Brusteinheit. Der
          KY-005 sitzt in der Pistolenmündung.
        </li>
        <li>
          <strong>Powerbank anschließen.</strong> Jeder ESP zieht typisch &lt;&nbsp;200&nbsp;mA
          — selbst eine kleine 5000-mAh-Powerbank reicht für mehrere Stunden Spielzeit.
        </li>
      </ol>

      <figure>
        <img src="/papers/v1/Lasertag_Breadboard.jpg" alt="Steckbrett-Prototyp mit allen Komponenten" />
        <figcaption>Steckbrett-Prototyp während der Erstinbetriebnahme</figcaption>
      </figure>

      <figure>
        <img src="/papers/v1/Konzept-Weste.jpg" alt="Brustmodul mit ESP, Empfänger und Stromversorgung" />
        <figcaption>Brustmodul — ESP32 mit TSOP38238 und Statusverkabelung</figcaption>
      </figure>

      <figure>
        <img src="/papers/v1/Konzept_Pistole.jpg" alt="Laserpistole mit OLED-Display und Button" />
        <figcaption>Laserpistole — KY-005 in der Mündung, OLED + Button am Griff</figcaption>
      </figure>

      <p>
        Für eine ausführlichere Erläuterung mit Anschluss-Diagrammen aller getesteten
        Alternativ-Komponenten siehe <a href="/wiki/research/paper-v1">Paper V1</a>.
      </p>
    </article>
  );
}
