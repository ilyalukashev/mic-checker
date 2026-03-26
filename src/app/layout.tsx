import type { Metadata } from "next";
import "./globals.css";

export const metadata: Metadata = {
  title: "Mic Authenticator — Ilia Lukashev",
  description: "Verify if your U87 or TLM103 microphone is genuine",
};

export default function RootLayout({
  children,
}: {
  children: React.ReactNode;
}) {
  return (
    <html lang="en">
      <body className="min-h-screen bg-black text-gray-100 antialiased">
        {children}
      </body>
    </html>
  );
}
