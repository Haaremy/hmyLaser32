export default function BuildFirmwarePage() {
  return (
    <article className="paper-prose">
      <h1>Firmware flashen</h1>
      <p>
        Der gesamte Arduino-Code liegt im GitHub-Repository unter{' '}
        <a href="https://github.com/Haaremy/hmyLaser32/tree/main/Arduino" rel="noopener noreferrer">
          <code>Arduino/</code>
        </a>{' '}und ist in zwei modulare Sketches aufgeteilt — einer pro Rolle.
      </p>

      <h2>Werkzeuge</h2>
      <ul>
        <li>Arduino IDE 2.x oder PlatformIO</li>
        <li>ESP32 Board-Support (über Board-Manager → „esp32 by Espressif Systems")</li>
        <li>Bibliotheken (in der IDE über Library Manager installieren):
          <ul>
            <li><code>IRremoteESP8266</code> (für Client — IR-Send/Receive mit NEC)</li>
            <li><code>U8g2</code> (für Client — OLED)</li>
          </ul>
        </li>
      </ul>

      <h2>Upload-Settings</h2>
      <table>
        <thead><tr><th>Setting</th><th>Wert</th></tr></thead>
        <tbody>
          <tr><td>Board</td><td>ESP32 Dev Module / NodeMCU-32S</td></tr>
          <tr><td>Upload Speed</td><td className="num">115200</td></tr>
          <tr><td>Flash Frequency</td><td className="num">40&nbsp;MHz</td></tr>
          <tr><td>Erase all Flash on upload</td><td><strong>true</strong></td></tr>
          <tr><td>Partition Scheme</td><td>Default 4MB with spiffs</td></tr>
        </tbody>
      </table>

      <h2>Client-Sketch (`Arduino/lasertag_client/lasertag_client.ino`)</h2>
      <p>Modulares Setup — alle Spielerlogik ist auf separate Dateien aufgeteilt:</p>
      <table>
        <thead><tr><th>Datei</th><th>Aufgabe</th></tr></thead>
        <tbody>
          <tr><td><code>lasertag_client.ino</code></td><td>Setup, Hauptloop, Lib-Includes</td></tr>
          <tr><td><code>Config.h</code></td><td>Pin-Belegung, Spielerdaten, IR-Codes, Konstanten</td></tr>
          <tr><td><code>Types.h</code></td><td>Message-Struktur für ESP-NOW</td></tr>
          <tr><td><code>Globals.h</code></td><td>Externe Variablendeklarationen</td></tr>
          <tr><td><code>Game.cpp/.h</code></td><td>Trigger-Handling, IR-Receive, Punkt-Vergabe, Respawn</td></tr>
          <tr><td><code>Network.cpp</code> / <code>LasertagNetwork.h</code></td><td>ESP-NOW Send/Receive Callbacks, Peer-Mgmt, Table-Broadcast</td></tr>
          <tr><td><code>Ranking.cpp/.h</code></td><td>Anti-Entropy-Algorithmus, Sortierung, Diff-Tracking</td></tr>
          <tr><td><code>Display.cpp/.h</code></td><td>OLED-Anzeige mit u8g2</td></tr>
          <tr><td><code>Led.cpp/.h</code></td><td>RGB-Status-LED</td></tr>
        </tbody>
      </table>

      <p>
        <strong>Pro Spieler ändern</strong> in <code>Config.h</code>:
      </p>
      <pre>{`constexpr char MY_NAME[] = "PLAYER_1";       // Anzeigename
constexpr uint8_t MY_IR_COMMAND = 0x01;       // 0x01..0xFE pro Gerät eindeutig`}</pre>

      <p>
        Beim zweiten ESP <code>"PLAYER_2"</code> und <code>0x02</code>, beim dritten
        <code>"PLAYER_3"</code> und <code>0x03</code> usw. — die IR-Codes müssen
        eindeutig sein, sonst zählt der Empfänger den Schuss falsch zu.
      </p>

      <h2>Server-Sketch (`Arduino/lasertag_server/lasertag_server.ino`)</h2>
      <p>
        Der Server-ESP braucht <em>keine</em> der Client-Komponenten — nur den
        NodeMCU-32S und eine Powerbank. Er übernimmt:
      </p>
      <ul>
        <li>WLAN-Setup über Captive Portal (mit WLAN-Scan, persistente Speicherung)</li>
        <li>Self-Registration beim Web-Portal <code>laser32.haaremy.de</code></li>
        <li>ESP-NOW Mithören aller Client-Tabellen</li>
        <li>Score-Diff-Erkennung → Hit-Forwarding</li>
        <li>Match-Orchestrierung mit Lobby-Timer</li>
      </ul>

      <h3>Module</h3>
      <table>
        <thead><tr><th>Datei</th><th>Aufgabe</th></tr></thead>
        <tbody>
          <tr><td><code>lasertag_server.ino</code></td><td>Setup, Hauptloop</td></tr>
          <tr><td><code>Config.h</code></td><td>Channel, Portal-URL, Timer-Defaults</td></tr>
          <tr><td><code>Storage.cpp/.h</code></td><td>NVS-Wrapper (Preferences-Lib)</td></tr>
          <tr><td><code>Identity.cpp/.h</code></td><td>Name + PIN beim ersten Boot generieren, persist</td></tr>
          <tr><td><code>WiFiSetup.cpp/.h</code></td><td>STA-Connect + Captive-AP + WLAN-Scan</td></tr>
          <tr><td><code>Portal.cpp/.h</code></td><td>Lokaler Webserver (Captive Portal + Settings + Match-Trigger)</td></tr>
          <tr><td><code>EspNow.cpp/.h</code></td><td>ESP-NOW-Listener + Score-Diff-Detector</td></tr>
          <tr><td><code>Bridge.cpp/.h</code></td><td>HTTPS-Client zum Web-Portal (Register, Heartbeat, Hit, Match-Lifecycle)</td></tr>
          <tr><td><code>Match.cpp/.h</code></td><td>State-Machine: IDLE → LOBBY → ACTIVE → DONE</td></tr>
        </tbody>
      </table>

      <h2>Erster Boot des Servers</h2>
      <ol>
        <li>Server-ESP flashen, mit Powerbank anschließen.</li>
        <li>Im Smartphone-WLAN nach <code>hmyLaser32-XXXX</code> suchen, verbinden (offen, kein Passwort).</li>
        <li>iOS/Android öffnen automatisch <code>http://192.168.4.1</code> als Captive Portal.</li>
        <li>WLAN scannen, eigenes WLAN wählen, Passwort eingeben → speichern.</li>
        <li>ESP rebootet, verbindet sich, registriert sich beim Portal.</li>
        <li>Den <strong>PIN</strong> aus der lokalen Seite merken — er ist für Match-Start
        und Settings-Edit nötig (sowohl lokal als auch im Web-Portal).</li>
      </ol>

      <h2>Erweiterung mit OLED (optional)</h2>
      <p>
        Der aktuelle Server-Code läuft headless. Wer ein OLED dranhängen will, kann das
        Client-Display-Modul übernehmen — auf der Setup-Seite hilfreich, um den PIN
        nach dem Boot direkt am Gerät abzulesen.
      </p>
    </article>
  );
}
