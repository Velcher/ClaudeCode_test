"use client";
import { useState } from "react";

interface Props { filename?: string; language?: string; children: string; }

export default function CodeBlock({ filename, language, children }: Props) {
  const [copied, setCopied] = useState(false);
  const handleCopy = async () => {
    await navigator.clipboard.writeText(children);
    setCopied(true);
    setTimeout(() => setCopied(false), 2000);
  };
  return (
    <div className="rounded-card border border-sage/10 overflow-hidden mb-4 bg-[#f5f2ea]">
      <div className="flex justify-between items-center px-4 py-2 bg-sage/5 font-mono text-[10px] text-sage/50">
        <span>{filename || language || "code"}</span>
        <button onClick={handleCopy} className="text-sage/30 hover:text-sage/60 transition-colors bg-transparent border-none cursor-pointer font-mono text-[10px]">{copied ? "copied!" : "copy"}</button>
      </div>
      <pre className="px-4 py-3 m-0 overflow-x-auto font-mono text-[11px] leading-relaxed text-olive/80">
        <code className={`language-${language || "text"}`}>{children}</code>
      </pre>
    </div>
  );
}
