import type { Metadata } from "next";
import PageTransition from "@/components/PageTransition";
export const metadata: Metadata = { title: "About" };

const skills = ["C", "ARM Assembly", "SME", "FMoPA", "Verilog", "矩阵计算", "性能调优", "嵌入式开发"];

export default function AboutPage() {
  return (
    <PageTransition>
    <div className="max-w-[640px] mx-auto px-6 py-12">
      <p className="font-mono text-[10px] text-sage/50 tracking-[3px] mb-4">&gt; ABOUT_ME_</p>
      <h1 className="text-2xl font-bold text-olive mb-6">关于我</h1>
      <div className="rounded-card border border-sage/10 bg-paper-dark/50 p-6 mb-6">
        <p className="text-[14px] text-olive-light/70 leading-relaxed font-light">底层计算爱好者，专注于 ARM 架构与矩阵计算。喜欢深挖指令集背后的硬件设计哲学，用代码理解每一颗芯片的呼吸。</p>
      </div>
      <h2 className="text-lg font-semibold text-olive mb-3">技术栈</h2>
      <div className="flex flex-wrap gap-2 mb-8">
        {skills.map((skill, i) => {
          const cfg = ["bg-sage/10 text-sage-dark border-sage/15", "bg-amber/10 text-amber border-amber/15", "bg-teal/10 text-teal border-teal/15"][i % 3];
          return (<span key={skill} className={`px-3 py-1 rounded-tag text-xs font-medium border ${cfg}`}>{skill}</span>);
        })}
      </div>
      <h2 className="text-lg font-semibold text-olive mb-3">链接</h2>
      <div className="flex gap-4">
        <a href="https://github.com/Velcher" target="_blank" rel="noopener noreferrer" className="text-sage/60 text-sm no-underline hover:text-sage transition-colors">GitHub</a>
      </div>
    </div>
    </PageTransition>
  );
}
