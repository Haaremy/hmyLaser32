import { getTranslations } from 'next-intl/server';

export default async function ArchitecturePage() {
  const t = await getTranslations('wiki');
  return (
    <>
      <h1>{t('architecture')}</h1>

      <div className="diagram-placeholder" aria-label="System architecture diagram placeholder">
        {t('diagram_placeholder')} — ESP-NOW (lokal) ↔ ESP32-Server ↔ Website (WLAN, REST + WebSocket)
      </div>

      <h2>Lokaler Spielfluss (ESP-NOW)</h2>
      <p>
        Clients senden Hit-Events Peer-to-Peer an den ESP32-Server. Kein Router nötig. Server hält die
        Spielzeit, weist Teams zu und broadcastet Spielzustand.
      </p>

      <h2>Online-Bridge (REST + WebSocket)</h2>
      <p>
        Sobald der ESP32-Server WLAN-Verbindung hat, sendet er Match-Start/End-Daten an dieses Portal
        und öffnet einen WebSocket für Live-Daten. Browser-Clients auf <code>/game</code> abonnieren
        denselben WebSocket.
      </p>

      <h2>Nachrichten-Typen</h2>
      <ul>
        <li><code>GAME_START</code> — Match-Beginn, Teams + Modus</li>
        <li><code>GAME_END</code> — Match-Ende, finale Statistik</li>
        <li><code>TEAM_ASSIGN</code> — Spieler zu Team zuordnen</li>
        <li><code>HIT_EVENT</code> — Einzelner Treffer (Schütze + Ziel + Zeitstempel)</li>
        <li><code>STATUS</code> — Periodischer Heartbeat</li>
        <li><code>SYNC_RESULT</code> — Server-Sync-Bestätigung</li>
      </ul>
    </>
  );
}
