import { getTranslations } from 'next-intl/server';

export default async function HardwarePage() {
  const t = await getTranslations('wiki');
  return (
    <>
      <h1>{t('hardware')}</h1>

      <h2>ESP32 Client</h2>
      <div className="diagram-placeholder" aria-label="ESP32 Client Schematic placeholder">
        {t('diagram_placeholder')} — IR LED (940nm) · TSOP receiver · 0.96″ OLED · LiPo battery
      </div>
      <ul>
        <li>IR-Sender: 940 nm LED (TSAL6200), Vorwiderstand 47 Ω, getrieben über IRLZ44 MOSFET via GPIO 16.</li>
        <li>IR-Empfänger: TSOP4838 (38 kHz) auf GPIO 17, 100 nF Stützkondensator zwischen VCC und GND.</li>
        <li>OLED: SSD1306 0.96″ I²C (SDA GPIO 21, SCL GPIO 22).</li>
        <li>LiPo: 3.7 V 1S, TP4056 Charger-Modul, MT3608 Step-Up auf 5 V für OLED.</li>
      </ul>

      <h2>ESP32 Server</h2>
      <div className="diagram-placeholder" aria-label="ESP32 Server Schematic placeholder">
        {t('diagram_placeholder')} — ESP32-DevKit · WiFi AP-Modus · ESP-NOW Coordinator
      </div>
      <ul>
        <li>Baugleich zum Client, ohne IR-Bauteile.</li>
        <li>WiFi-AP-Modus für lokales Captive-Portal-Setup.</li>
        <li>Optional Ethernet via WT32-ETH01 für Online-Bridge-Anbindung.</li>
      </ul>
    </>
  );
}
