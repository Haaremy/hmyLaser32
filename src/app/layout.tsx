import type { Metadata } from 'next';
import { NextIntlClientProvider } from 'next-intl';
import { getLocale, getMessages } from 'next-intl/server';
import { Header } from '@/components/Header';
import { Footer } from '@/components/Footer';
import './globals.css';

export const metadata: Metadata = {
  title: 'hmyLaser32 — DIY Lasertag',
  description: 'DIY Lasertag system based on ESP32 — online bridge & wiki',
  robots: { index: true, follow: true }
};

export default async function RootLayout({ children }: { children: React.ReactNode }) {
  const locale = await getLocale();
  const messages = await getMessages();
  return (
    <html lang={locale}>
      <body>
        <NextIntlClientProvider locale={locale} messages={messages}>
          <Header />
          <main>
            <div className="container">{children}</div>
          </main>
          <Footer />
        </NextIntlClientProvider>
      </body>
    </html>
  );
}
