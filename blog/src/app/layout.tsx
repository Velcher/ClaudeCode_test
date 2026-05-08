import type { Metadata, Viewport } from "next";
import "./globals.css";

export const viewport: Viewport = {
  width: "device-width",
  initialScale: 1,
};

export const metadata: Metadata = {
  title: {
    default: "FMoPA.Blog — 深入底层计算",
    template: "%s | FMoPA.Blog",
  },
  description: "ARM SME · FMoPA 指令 · 矩阵乘法 · 性能调优",
};

export default function RootLayout({ children }: { children: React.ReactNode }) {
  return (
    <html lang="zh-CN">
      <body className="min-h-screen flex flex-col">
        <Nav />
        <main className="flex-1">{children}</main>
        <Footer />
      </body>
    </html>
  );
}

function Nav() {
  return (
    <nav className="sticky top-0 z-50 bg-paper/85 backdrop-blur-md border-b border-sage/15">
      <div className="max-w-3xl mx-auto px-4 sm:px-6 h-12 sm:h-14 flex items-center justify-between">
        <a href="/" className="font-mono font-semibold text-sage-dark no-underline text-base sm:text-lg">FMoPA.Blog</a>
        <div className="flex gap-4 sm:gap-6 text-xs sm:text-sm font-medium">
          <a href="/" className="text-sage no-underline">Posts</a>
          <a href="/about" className="text-olive-light/50 no-underline hover:text-olive-light/70 transition-colors">About</a>
        </div>
      </div>
    </nav>
  );
}

function Footer() {
  return (
    <footer className="border-t border-amber/10 bg-paper-dark/50">
      <div className="max-w-3xl mx-auto px-4 sm:px-6 h-12 flex items-center justify-between text-[9px] sm:text-[10px] text-olive-light/40 gap-2">
        <span>&copy; 2026 FMoPA.Blog</span>
        <span className="font-mono hidden xs:inline">sys.online</span>
        <span>Built with Next.js</span>
      </div>
    </footer>
  );
}
