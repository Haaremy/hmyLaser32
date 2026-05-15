import { getTranslations } from 'next-intl/server';

export default async function ArchitecturePage() {
  const t = await getTranslations('wiki');
  return (
    <article className="paper-prose">
      <h1>{t('architecture')}</h1>
      <p style={{ color: 'var(--color-text-muted)', fontSize: '0.9em' }}>
        Aus: <em>Entwicklung eines lokalen Multiplayer Lasertag Games</em>, Jeremy Becker.
        Volltext im <a href="/papers/Lasertag_Paper_V1.pdf">{t('paper_v1')}</a>.
      </p>

      <h2>Konzept</h2>
      <p>
        Ziel ist eine Spielerfahrung ähnlich kommerzieller Lasertag-Systeme, reduziert auf einen
        konzeptionellen Prototypen. Der Spieler wird mit einer Weste oder vergleichbarem
        Trägermaterial für die Empfangseinheit sowie mit einer Laserpistole als Sendeeinheit
        ausgestattet. Die Sendeeinheit überträgt per Knopfdruck ein Infrarotsignal mit Spieler-ID;
        die Empfangseinheit erfasst dieses Signal und ein Mikrocontroller wertet es aus. Der
        Datenaustausch zwischen den Spieleinheiten erfolgt drahtlos — Offline-Betrieb ohne zentralen
        Server ist möglich.
      </p>

      <h3>Anforderungen</h3>
      <ul>
        <li>Erkennung von Infrarotsignalen mit ausreichender Reichweite für kurze bis mittlere Spielentfernungen</li>
        <li>Eindeutige Zuordnung empfangener Signale zum sendenden Spieler</li>
        <li>Drahtlose Kommunikation zwischen Spieleinheiten ohne Access Point oder zentralen Server</li>
        <li>Lokale Verarbeitung und Anzeige spielrelevanter Daten</li>
        <li>Umsetzung mit kostengünstigen und frei verfügbaren Komponenten</li>
      </ul>

      <h2>Software-Architektur</h2>

      <h3>Steuersignale</h3>
      <p>
        Ein Button am ESP wird kontinuierlich überwacht. Wird das Signal als neuer Tastendruck erkannt,
        löst es die Funktion für den Infrarot-Lichtimpuls aus. Der 32-Bit-Hexwert dient hier alleinig
        der Erkennung des sendenden Spielers — er könnte erweitert werden, um mehr Spielinformationen
        zu übertragen.
      </p>
      <pre>{`bool reading = digitalRead(BUTTON_PIN);

if (reading != lastButtonState && millis() - lastDebounce > DEBOUNCE_MS) {
    lastDebounce = millis();
    lastButtonState = reading;

    if (reading == LOW) {
        irsend.sendNEC(IR_SHOT_CODE, 32);
        shotsFired++;
        updateDisplay();
    }
}`}</pre>

      <h3>Punkteauswertung</h3>
      <p>
        Auf der Empfängerseite werden ein oder mehrere Infrarotempfänger kontinuierlich überwacht.
        Wird ein erwartetes Signal dekodiert, wird der Treffer registriert und die Punkteberechnung
        ausgelöst:
      </p>
      <pre>{`if (irrecv.decode(&results)) {
    if (results.value == 0xFF00FF) {
        hitCount++;
    }
    irrecv.resume();
}`}</pre>

      <h3>Datenaustausch — ESP-NOW + Gossip</h3>
      <p>
        Nach dem Senden ist es dem Sender nicht möglich zu prüfen, ob ein Treffer landete. Daher
        muss dem Sender der aktuelle Datenstand des getroffenen Spielers übermittelt werden — drahtlos,
        ohne zentralen Server. Der ESP32 bietet ab Boardversion 3 das <strong>ESP-NOW-Protokoll</strong>:
        Peer-to-Peer-Kommunikation auf dem 2,4-GHz-WLAN-Band, direkt zwischen ESP-Geräten ohne Router.
      </p>
      <p>
        Auf Grundlage von ESP-NOW kommt ein <strong>Gossip-ähnliches Protokoll</strong> zum Einsatz.
        Es ist ein dezentrales Verfahren zum Datenaustausch — Informationen werden schrittweise
        zwischen Netzwerkknoten weitergegeben, ähnlich der Verbreitung in sozialen Gruppen. Unter
        Nutzung von <em>Anti-Entropy</em> synchronisieren die Nodes (alle erreichbaren ESPs) ihre
        Daten und patchen inhaltliche, veraltete Abweichungen.
      </p>

      <blockquote>
        Im Projekt bedeutet das: Der Empfänger eines Treffers passt seine Daten an, berechnet die
        neuen Punktewerte für den Sender und broadcastet das aktuelle Punkte-Array mit Zeitstempeln.
        Andere Geräte vergleichen Einträge: bekannt → ignorieren; neuer → übernehmen + weiter
        broadcasten; veraltet → eigene Version broadcasten zum Update des Senders.
      </blockquote>

      <h3>Nachrichten-Typen (Bridge zur Website)</h3>
      <p>
        Sobald der ESP-Server WLAN-Konnektivität hat, sendet er Match-Lifecycle-Events an dieses Portal:
      </p>
      <ul>
        <li><code>POST /api/bridge/register</code> — Self-Registration mit (Name, PIN)</li>
        <li><code>POST /api/bridge/match/start</code> — Match-Beginn, Teams + Modus</li>
        <li><code>POST /api/bridge/hit</code> — Einzeltreffer (Schütze, Ziel, Punkte, ts)</li>
        <li><code>POST /api/bridge/match/end</code> — Match-Ende, aggregierte Statistik pro Spieler</li>
        <li><code>WS /api/bridge/ws?token=&lt;pin&gt;</code> — Live-Stream</li>
      </ul>
    </article>
  );
}
