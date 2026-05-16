export default function PaperV2Page() {
  return (
    <article className="hmy-lt-paper">
      <h1>Paper V2 — Erweiterung</h1>
      <p style={{ color: 'var(--color-text-muted)' }}>In Arbeit. Diese Seite wird befüllt, sobald die Erweiterungsarbeit veröffentlicht ist.</p>

      <div className="hmy-card hmy-card__body" style={{ marginTop: '2rem' }}>
        <h3 style={{ marginTop: 0 }}>Geplante Themen</h3>
        <ul>
          <li>Online-Bridge zum Web-Portal (Implementiert in hmyLaser32 v0.2)</li>
          <li>Persistente Spielerprofile mit NFC-Token</li>
          <li>Gehäuse-Design und mechanische Integration</li>
          <li>Akustische und visuelle Feedback-Erweiterungen</li>
          <li>Vergleich: Gossip-Protokoll vs. zentrale Bridge</li>
        </ul>
      </div>

      <p>
        Bis dahin: siehe <a href="/research/paper-v1">Paper V1</a> und die{' '}
        <a href="/diy">aktuelle Bauanleitung</a>.
      </p>
    </article>
  );
}
