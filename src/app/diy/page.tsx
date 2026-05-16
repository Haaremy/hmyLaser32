import { getTranslations } from 'next-intl/server';

type Component = { name: string; qty: string; hint: string; source: string };

// Hinweis: Die Bezugsquellen sind die tatsächlich vom Autor (Jeremy Becker)
// für den V1-Prototyp verwendeten Produkte. Andere kompatible Module
// funktionieren ebenfalls — bitte technische Eckdaten vergleichen.
const components: Component[] = [
  {
    name: 'ESP32 NodeMCU-32S',
    qty: '1×',
    hint: 'Dual-Core 240 MHz, WLAN, ESP-NOW, per USB programmierbar. Spar-Tipp: Mehrpack — pro Spieler braucht es eins, plus optional ein Server-ESP.',
    source: 'https://www.amazon.de/dp/B0D8635YZ6'
  },
  {
    name: 'TSOP38238 IR-Empfänger',
    qty: '1×',
    hint: 'Bis 45 m Reichweite, 38 kHz. Auf das Brustmodul nach vorne.',
    source: 'https://www.tme.eu/de/details/tsop38238/ir-empfangermodule/vishay/'
  },
  {
    name: 'KY-005 IR-Sender-Modul',
    qty: '1×',
    hint: '940 nm, direkt am ESP-GPIO ohne MOSFET schaltbar. Sitzt in der Pistolenmündung.',
    source: 'https://www.amazon.de/dp/B0F9L3SZ97'
  },
  {
    name: 'SSD1306 OLED 0.96″ I²C',
    qty: '1×',
    hint: '128 × 64 Pixel, zeigt Name / Score / Rank.',
    source: 'https://www.amazon.de/dp/B01L9GC470'
  },
  {
    name: 'Taster (Mikroschalter)',
    qty: '1×',
    hint: 'Auslöseknopf an Pistole. Pull-Up auf GPIO 19.',
    source: 'https://www.amazon.de/dp/B07XWYHPZH'
  },
  {
    name: '5 mm RGB-LED, gemeinsame Kathode',
    qty: '1×',
    hint: 'Status-LED am Brustmodul. 3 PWM-Pins (25/26/27).',
    source: 'https://www.amazon.de/dp/B0897LDR9N'
  },
  {
    name: 'Widerstand 330 Ω',
    qty: '3×',
    hint: 'Vorwiderstand pro Farbkanal der RGB-LED.',
    source: 'https://www.reichelt.de/de/de/widerstand-1-4-w-5-330-ohm-1-4w-330-p1408.html'
  },
  {
    name: 'Powerbank 5 V USB',
    qty: '1×',
    hint: 'Beliebige Standard-Powerbank. ESP zieht typisch < 200 mA.',
    source: ''
  },
  {
    name: 'Lochrasterplatine',
    qty: '1×',
    hint: 'Steckbrett-Prototyp zuerst, dann auf Loch­platine löten.',
    source: 'https://www.reichelt.de/de/de/inhalt.html?ACTION=446&q=lochrasterplatine'
  },
  {
    name: 'Kupferkabel + Lötzinn',
    qty: 'nach Bedarf',
    hint: 'Signal- und Versorgungsleitungen.',
    source: ''
  },
  {
    name: 'Weste / Trägermaterial',
    qty: '1×',
    hint: 'Befestigung des Brustmoduls mit IR-Empfänger. Im V1 wurde eine taktische Weste mit MOLLE-Schlaufen verwendet.',
    source: 'https://www.amazon.de/dp/B0F6Y2MSXN'
  },
  {
    name: 'MFRC522 NFC-Reader (optional)',
    qty: '1×',
    hint: 'Für Account-Binding via NFC-Karte. SPI: VSPI (SCK 18 / MISO 19 / MOSI 23 / SS 5 / RST 33). Bei Verwendung muss BUTTON_PIN von 19 auf 32 verlegt werden.',
    source: ''
  },
  {
    name: 'NFC-Karte (Mifare Classic 1K, optional)',
    qty: '1×',
    hint: 'Speichert username|token aus dem Account.',
    source: 'https://www.reichelt.de/de/de/inhalt.html?ACTION=446&q=mifare%201k%20karte'
  }
];

const tools = [
  'Lötkolben + Lötzinn',
  'Seitenschneider',
  'Abisolierzange',
  'Schraubenzieher (Kreuz, klein)',
  'Multimeter (optional, zum Debuggen)',
  'Computer mit Arduino IDE oder PlatformIO'
];

const steps = [
  {
    title: 'Werkstatt vorbereiten',
    body: (
      <>
        <p>
          Lege alle Komponenten aus der Liste bereit. Arduino IDE öffnen,
          ESP32-Board-Support installieren (Board-Manager → „esp32 by Espressif Systems").
          Bibliotheken über den Library Manager installieren: <code className="hmy-code">IRremoteESP8266</code> und <code className="hmy-code">U8g2</code>.
        </p>
      </>
    )
  },
  {
    title: 'Steckbrett-Prototyp',
    body: (
      <>
        <p>
          Zuerst alles auf dem Breadboard testen — der ESP32 ist mittig, von Ground und
          3,3 V führen Leiterbahnen zu den Komponenten. Pin-Belegung (steht auch in <code className="hmy-code">Arduino/lasertag_client/Config.h</code>):
        </p>
        <table>
          <thead><tr><th>GPIO</th><th>Funktion</th><th>Komponente</th></tr></thead>
          <tbody>
            <tr><td className="num">4</td><td>IR-Sender Out</td><td>KY-005 Signal</td></tr>
            <tr><td className="num">14</td><td>IR-Receiver In</td><td>TSOP38238 Signal</td></tr>
            <tr><td className="num">19</td><td>Button (Pull-Up)</td><td>Auslöseknopf</td></tr>
            <tr><td className="num">21</td><td>I²C SDA</td><td>OLED</td></tr>
            <tr><td className="num">22</td><td>I²C SCL</td><td>OLED</td></tr>
            <tr><td className="num">25/26/27</td><td>PWM R/G/B</td><td>RGB-LED über 330 Ω</td></tr>
          </tbody>
        </table>
      </>
    )
  },
  {
    title: 'Funktionstest am Steckbrett',
    body: (
      <>
        <p>
          Das Client-Sketch <code className="hmy-code">Arduino/lasertag_client/lasertag_client.ino</code> aus
          dem <a href="https://github.com/Haaremy/hmyLaser32/tree/main/Arduino" rel="noopener noreferrer">GitHub-Repo</a> öffnen.
          In <code className="hmy-code">Config.h</code> Spielerdaten setzen — pro Gerät eindeutig:
        </p>
        <pre>{`constexpr char MY_NAME[]      = "PLAYER_1";  // pro ESP unterschiedlich
constexpr uint8_t MY_IR_COMMAND = 0x01;       // 0x01..0xFE pro Gerät`}</pre>
        <p>
          Flashen mit <strong>Erase all Flash on upload = true</strong>, Upload-Speed 115 200,
          Flash 40 MHz. Auf dem OLED sollte Name + Score erscheinen. Button drücken →
          IR-LED blinkt kurz, OLED zeigt Schüsse hoch.
        </p>
      </>
    )
  },
  {
    title: 'Auf Lochplatine löten',
    body: (
      <p>
        Wenn der Prototyp läuft: alles auf eine Lochplatine übertragen.
        Sauberes Löten ist hier entscheidend — schlechte GND-Verbindungen sind
        die häufigste Fehlerquelle. Die RGB-LED-Kathode (längster Pin) an GND, die
        drei Farb-Pins über je 330 Ω an GPIO 25 / 26 / 27.
      </p>
    )
  },
  {
    title: 'Brustmodul + Pistole zusammenbauen',
    body: (
      <>
        <p>
          <strong>Brustmodul:</strong> ESP32 + TSOP38238 + RGB-LED + Powerbank-USB-Anschluss
          auf eine Trägerplatte. TSOP zeigt nach vorne (Treffer-Richtung).
        </p>
        <p>
          <strong>Pistole:</strong> KY-005 + Button + OLED + Verbindungskabel zum Brustmodul.
          Der KY-005 sitzt in der Pistolenmündung.
        </p>
        <p>
          <strong>V1 hat noch kein Gehäuse</strong> — die Komponenten sind offen auf der
          Lochplatine montiert. Mechanisches Design ist Teil von <a href="/research/paper-v2">Paper V2</a>.
        </p>
      </>
    )
  },
  {
    title: 'Server-ESP aufsetzen',
    body: (
      <>
        <p>
          Zusätzlich zu den Spieler-Sets braucht es einen <strong>Server-ESP</strong> —
          ein eigener NodeMCU-32S an einer Powerbank, ganz ohne IR-Hardware. Er bildet die
          Brücke zwischen den Clients (ESP-NOW) und dem Web-Portal (laser32.haaremy.de).
        </p>
        <p>
          Sketch <code className="hmy-code">Arduino/lasertag_server/lasertag_server.ino</code> flashen.
          Beim ersten Boot öffnet er einen offenen AP <code className="hmy-code">hmyLaser32-XXXX</code>.
          Mit dem Smartphone verbinden, im Captive Portal das WLAN scannen, eigenes Netz auswählen,
          Passwort eingeben → speichern. ESP rebootet, verbindet sich, registriert sich automatisch.
          Den angezeigten <strong>PIN</strong> merken.
        </p>
      </>
    )
  },
  {
    title: 'Erstes Spiel',
    body: (
      <>
        <p>
          Alle Spieler-ESPs einschalten (Powerbank anschließen). Auf der <a href="/">Startseite</a>{' '}
          erscheint der Server-ESP. Auf der lokalen Setup-Seite (Server-IP) oder über
          das Web-Interface (Match-Detail → Tab „Einstellungen") <strong>Lobby-Zeit</strong>,
          <strong> Match-Dauer</strong> und <strong>Modus</strong> festlegen.
        </p>
        <p>
          Match starten — es beginnt eine Lobby-Phase (Spieler verteilen sich), danach
          schaltet der Server automatisch live. Treffer landen sofort im Live-Feed des
          Web-Portals. Nach Ablauf der Match-Dauer wird das Endergebnis übermittelt
          und landet in den <a href="/champions">Champions</a>-Statistiken.
        </p>
      </>
    )
  }
];

export default async function DiyPage() {
  const t = await getTranslations('diy');
  return (
    <div className="hmy-lt-recipe">
      <h1>{t('title')}</h1>
      <p className="hmy-lt-recipe__intro">{t('subtitle')}</p>

      <div className="hmy-alert hmy-alert--info">{t('based_on_v1')}</div>

      <div className="hmy-lt-recipe__meta">
        <div className="hmy-lt-recipe__meta-item">
          <div className="hmy-lt-recipe__meta-label">{t('meta_difficulty')}</div>
          <div className="hmy-lt-recipe__meta-value">{t('meta_difficulty_value')}</div>
        </div>
        <div className="hmy-lt-recipe__meta-item">
          <div className="hmy-lt-recipe__meta-label">{t('meta_time')}</div>
          <div className="hmy-lt-recipe__meta-value">{t('meta_time_value')}</div>
        </div>
        <div className="hmy-lt-recipe__meta-item">
          <div className="hmy-lt-recipe__meta-label">{t('meta_cost')}</div>
          <div className="hmy-lt-recipe__meta-value">{t('meta_cost_value')}</div>
        </div>
        <div className="hmy-lt-recipe__meta-item">
          <div className="hmy-lt-recipe__meta-label">{t('meta_players')}</div>
          <div className="hmy-lt-recipe__meta-value">{t('meta_players_value')}</div>
        </div>
      </div>

      <h2>{t('ingredients_heading')}</h2>
      <ul className="hmy-lt-ingredients">
        {components.map((c) => (
          <li className="hmy-lt-ingredient" key={c.name}>
            <div>
              <div className="hmy-lt-ingredient__name">{c.name}</div>
              <div className="hmy-lt-ingredient__hint">{c.hint}</div>
              {c.source && (
                <div className="hmy-lt-ingredient__source">
                  <a href={c.source} rel="noopener noreferrer">{t('source')} →</a>
                </div>
              )}
            </div>
            <div className="hmy-lt-ingredient__qty">{c.qty}</div>
          </li>
        ))}
      </ul>

      <h2>{t('tools_heading')}</h2>
      <ul>
        {tools.map((tool) => <li key={tool}>{tool}</li>)}
      </ul>

      <h2>{t('steps_heading')}</h2>
      {steps.map((s, i) => (
        <div className="hmy-lt-step" key={i}>
          <div className="hmy-lt-step__num">{i + 1}</div>
          <div className="hmy-lt-step__body">
            <h3 className="hmy-lt-step__title">{s.title}</h3>
            {s.body}
          </div>
        </div>
      ))}
    </div>
  );
}
