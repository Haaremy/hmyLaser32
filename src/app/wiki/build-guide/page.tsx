import { getTranslations } from 'next-intl/server';

export default async function BuildGuidePage() {
  const t = await getTranslations('wiki');
  return (
    <article className="paper-prose">
      <h1>{t('build_guide')}</h1>
      <p style={{ color: 'var(--color-text-muted)', fontSize: '0.9em' }}>
        Aus: <em>Entwicklung eines lokalen Multiplayer Lasertag Games</em>, Jeremy Becker.
        Volltext im <a href="/papers/Lasertag_Paper_V1.pdf">{t('paper_v1')}</a>.
      </p>

      <h2>Add-On, Gehäuse und Trägermaterial</h2>
      <p>
        Das konzeptionelle Set besteht aus einer Laserpistole und einer Weste. Zur Demonstration
        werden ausschließlich funktionale Komponenten umgesetzt: ein OLED-Display zur Anzeige der
        Spielerstatistiken, ein Auslöseknopf für den Sender sowie ein Befestigungsmechanismus für die
        Sensoren an der Kleidung. Optionale Erweiterungen (3D-gedruckte Gehäuse, dekorative
        LED-Ringe) sind nicht Bestandteil des Prototyps.
      </p>

      <h2>Zusammenbau — Schritt für Schritt</h2>

      <ol>
        <li>
          <strong>Steckbrett-Prototyp aufbauen.</strong> Sensoren und Elektrokleinteile zunächst
          aufs Breadboard stecken. Funktioniert Button-Input, IR-Send, IR-Receive und das
          OLED-Display? Erst danach auf eine Lochplatine mit festen Lötstellen übertragen.
        </li>

        <li>
          <strong>Pin-Belegung beachten.</strong> Im Zentrum des Aufbaus sitzt der ESP32; von Ground
          und 3,3&nbsp;V führen Leiterbahnen zu den Komponenten:
          <ul>
            <li><code>Pin 19</code> — Button</li>
            <li><code>Pin 4</code> — Signalpin des Infrarotsenders</li>
            <li><code>Pin 15</code> — Signalpin des Empfängers</li>
            <li><code>Pin 21 / 22</code> — Datenpins (SDA/SCL) des OLED-Displays</li>
          </ul>
          Rote/orange Kabel sind die positive 3,3&nbsp;V-Versorgung, grüne/schwarze sind GND,
          die bunten Kabel decken Signalverbindungen ab.
        </li>

        <li>
          <strong>Lochplatine löten.</strong> Übertrage die Steckbrett-Konstruktion auf eine
          Lochplatine. Sauberes Löten ist hier kritisch: Ein fehlerhafter Zusammenbau kann
          Kommunikation, Stromversorgung oder Erdung beeinträchtigen und Messergebnisse verfälschen.
        </li>

        <li>
          <strong>Brustmodul montieren.</strong> ESP + Empfänger werden zum Westen-/Brustmodul. Der
          Empfänger ist frontal ausgerichtet.
        </li>

        <li>
          <strong>Laserpistole mit OLED + Sender bauen.</strong> Per Kabel werden Brustmodul und
          Laserpistole verbunden — zusammen ergeben sie das Lasertag-Set.
        </li>
      </ol>

      <figure>
        <img src="/papers/v1/Lasertag_Breadboard.jpg" alt="Steckbrett mit Komponenten" />
        <figcaption>{t('figure')} 1: Steckbrett-Prototyp mit ESP32, Button, IR-Sender, IR-Empfänger und OLED</figcaption>
      </figure>

      <figure>
        <img src="/papers/v1/Konzept-Set.jpg" alt="Komponenten für Weste und Laserpistole" />
        <figcaption>{t('figure')} 2: Komponenten für Weste und Laserpistole</figcaption>
      </figure>

      <figure>
        <img src="/papers/v1/Konzept-Weste.jpg" alt="Steuerkomponente ESP mit Empfänger als Brust-/Westenmodul" />
        <figcaption>{t('figure')} 3: Brustmodul — ESP mit Empfänger als Westenmodul</figcaption>
      </figure>

      <figure>
        <img src="/papers/v1/Konzept_Pistole.jpg" alt="Laserpistole mit OLED-Display, Infrarotsender und Button" />
        <figcaption>{t('figure')} 4: Laserpistole mit OLED-Display, Infrarotsender und Button</figcaption>
      </figure>

      <h2>Test-Ergebnisse (Auszug)</h2>
      <p>
        Im fensterlosen Kellerflur (kein natürliches Licht) wurden mit KY-005&nbsp;→&nbsp;TSOP38238 bis zu
        <strong>&nbsp;30&nbsp;m Reichweite</strong> erreicht. Bei direkter Sonneneinstrahlung sinkt die
        Reichweite auf ca. 2&nbsp;m. ESP-NOW kommuniziert outdoor zuverlässig bis 65&nbsp;m und in
        Innenräumen (gerade Flure) ebenfalls bis 50&nbsp;m. Details siehe Paper V1.
      </p>
    </article>
  );
}
