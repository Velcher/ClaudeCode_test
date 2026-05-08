import Link from "next/link";

export default function NotFound() {
  return (
    <div className="max-w-[480px] mx-auto px-6 py-20 text-center">
      <p className="font-mono text-6xl text-sage/15 mb-4">404</p>
      <p className="font-mono text-[10px] text-sage/40 tracking-[3px] mb-3">&gt; SIGNAL_LOST_</p>
      <h1 className="text-xl font-semibold text-olive mb-2">信号丢失</h1>
      <p className="text-[13px] text-olive-light/50 mb-6 font-light">这个页面不存在，可能已被移除或链接失效</p>
      <Link href="/" className="inline-block px-4 py-2 rounded-lg bg-sage/10 border border-sage/15 text-sage-dark text-sm no-underline hover:bg-sage/15 transition-colors">返回首页</Link>
    </div>
  );
}
