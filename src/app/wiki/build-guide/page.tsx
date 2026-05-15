import { getTranslations } from 'next-intl/server';

export default async function BuildGuidePage() {
  const t = await getTranslations('wiki');
  const steps = [
    { title: 'Firmware flashen', body: 'ESP32-Firmware aus Repo bauen (PlatformIO) und über USB flashen.' },
    { title: 'IR-Sender verdrahten', body: '940 nm IR-LED über MOSFET an GPIO 16, mit 47 Ω Vorwiderstand.' },
    { title: 'IR-Empfänger anschließen', body: 'TSOP4838 an GPIO 17, VCC + GND, 100 nF Stützkondensator.' },
    { title: 'OLED verkabeln', body: '0.96″ SSD1306 über I²C (SDA 21, SCL 22), 5 V Versorgung.' },
    { title: 'Gehäuse montieren', body: '3D-Druck-Vorlagen aus Repo, IR-LED in der „Mündung", TSOP frontal.' },
    { title: 'Captive Portal konfigurieren', body: 'ESP32-AP verbinden, Access-Code aus /admin/devices eintragen.' }
  ];
  return (
    <>
      <h1>{t('build_guide')}</h1>
      <ol>
        {steps.map((s, i) => (
          <li key={i} style={{ marginBottom: '1.5rem' }}>
            <h3 style={{ margin: '0 0 0.25rem' }}>{s.title}</h3>
            <p style={{ margin: 0 }}>{s.body}</p>
            <div className="diagram-placeholder" style={{ padding: '2rem', marginTop: '0.5rem' }}>
              {t('diagram_placeholder')}
            </div>
          </li>
        ))}
      </ol>
    </>
  );
}
