export default function BuildRunPage() {
  return (
    <article className="paper-prose">
      <h1>Erstes Spiel starten</h1>
      <p>
        Sobald die Hardware steht und die Firmware geflasht ist: so läuft das
        erste Match ab.
      </p>

      <h2>1. Spieler-ESPs einschalten</h2>
      <p>
        Jeden Client-ESP per Powerbank anschließen. Auf dem OLED erscheint
        Name, Punktestand und der Rank. Die RGB-LED leuchtet weiß (Standby).
      </p>

      <h2>2. Server-ESP einschalten</h2>
      <p>
        Server-ESP per Powerbank anschließen. Bei der ersten Inbetriebnahme:
        Captive Portal-Setup (siehe <a href="/wiki/build/firmware">Firmware-Seite</a>).
        Sonst verbindet er sich automatisch.
      </p>
      <p>
        Auf <a href="/">https://laser32.haaremy.de/</a> erscheint der Server als Karte —
        Status <strong>Online</strong>, kein laufendes Spiel.
      </p>

      <h2>3. Match-Einstellungen</h2>
      <p>Zwei Wege:</p>
      <ul>
        <li>
          <strong>Lokal</strong>: Mit Server-IP verbinden (steht in den Router-Geräten oder
          auf der lokalen Setup-Seite des Servers im Tab „Match-Einstellungen"). PIN eingeben,
          <em>Modus</em>, <em>Lobby-Sekunden</em> (Verteilzeit) und <em>Match-Sekunden</em> setzen.
        </li>
        <li>
          <strong>Über das Web-Portal</strong>: Auf{' '}
          <a href="/">laser32.haaremy.de</a> → Server-Karte anklicken → Match-Detail-Seite →
          Tab „Einstellungen" → PIN eingeben → Werte ändern.
        </li>
      </ul>

      <h2>4. Match starten</h2>
      <p>
        Auf der lokalen Setup-Seite des Servers: <strong>„Match starten"</strong> mit PIN bestätigen.
        Ab jetzt läuft die <strong>Lobby-Phase</strong> — Spieler verteilen sich, verstecken sich,
        wählen Positionen.
      </p>
      <p>
        Nach Ablauf der Lobby-Zeit wechselt der Server automatisch in die{' '}
        <strong>ACTIVE</strong>-Phase und meldet das Match beim Portal an. Auf der
        Match-Detail-Seite läuft jetzt der Countdown-Timer, Live-Feed wird
        aktualisiert sobald die ersten Treffer registriert sind.
      </p>

      <h2>5. Während des Spiels</h2>
      <p>
        Die Clients senden ihre Score-Tables periodisch via ESP-NOW. Der Server
        erkennt Punkt-Diffs (jemand hat einen Treffer gelandet → Punkte sind
        gestiegen) und leitet sie als <code>hit_event</code> an das Web-Portal weiter.
      </p>
      <p>
        Auf der Match-Detail-Seite:
      </p>
      <ul>
        <li>Countdown-Timer (rot bei &lt; 1 min, gelb bei &lt; 3 min)</li>
        <li>Leaderboard mit farbcodiertem Namen (Team-Farbe), K/D und Trefferquote</li>
        <li>Live-Feed mit Zeitstempel — neue Treffer blinken kurz auf</li>
      </ul>

      <h2>6. Match-Ende</h2>
      <p>
        Wenn die Match-Sekunden abgelaufen sind, schickt der Server automatisch das
        finale Ranking an <code>POST&nbsp;/api/bridge/match/end</code>. Der Match wechselt in
        den Status <strong>finished</strong>, das Web-Portal zeigt die finale Statistik —
        K/D, Punkte, Trefferquote pro Spieler.
      </p>

      <h2>Troubleshooting</h2>
      <table>
        <thead><tr><th>Symptom</th><th>Lösung</th></tr></thead>
        <tbody>
          <tr>
            <td>Server taucht nicht im Portal auf</td>
            <td>
              Im lokalen Setup-Web prüfen: WLAN verbunden? Portal-Registrierung erfolgreich?
              Falls Heartbeat fehlschlägt: PIN-Konflikt — Identität zurücksetzen.
            </td>
          </tr>
          <tr>
            <td>Treffer landen nicht im Live-Feed</td>
            <td>
              Alle Clients auf gleichem ESP-NOW-Kanal? <code>WIFI_CHANNEL=1</code> in beiden
              Configs. Falls dein Heim-WLAN auf Kanal 1 läuft: Router-Kanal ändern oder
              Server an festen Kanal binden.
            </td>
          </tr>
          <tr>
            <td>RGB-LED leuchtet falsche Farben</td>
            <td>Common-Anode statt Common-Cathode? <code>RGB_COMMON_ANODE=true</code> in <code>Config.h</code>.</td>
          </tr>
          <tr>
            <td>IR-Reichweite zu kurz</td>
            <td>Direkte Sonneneinstrahlung verkürzt die Reichweite stark (vgl. Paper V1 §&nbsp;Evaluation). Indoor 30 m sind realistisch, outdoor 2–10 m je nach Licht.</td>
          </tr>
        </tbody>
      </table>
    </article>
  );
}
