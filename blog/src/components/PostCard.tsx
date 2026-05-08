"use client";
import { motion } from "framer-motion";
import Link from "next/link";
import { Post } from "@/lib/posts";

const accentConfigs = [
  { icon: "▶", bg: "bg-sage-light/30", border: "border-sage/15", iconColor: "text-sage", dateColor: "text-sage/50", tagBg: "bg-sage/10", tagColor: "text-sage-dark", arrowColor: "text-sage/30", glow: "shadow-[0_2px_12px_rgba(100,120,80,0.04)]" },
  { icon: "◆", bg: "bg-amber-light/30", border: "border-amber/15", iconColor: "text-amber", dateColor: "text-amber/50", tagBg: "bg-amber/10", tagColor: "text-amber", arrowColor: "text-amber/30", glow: "shadow-[0_2px_12px_rgba(120,100,60,0.04)]" },
  { icon: "◇", bg: "bg-teal-light/30", border: "border-teal/15", iconColor: "text-teal", dateColor: "text-teal/50", tagBg: "bg-teal/10", tagColor: "text-teal", arrowColor: "text-teal/30", glow: "shadow-[0_2px_12px_rgba(80,110,120,0.04)]" },
];

interface Props { post: Post; index: number; }

export default function PostCard({ post, index }: Props) {
  const cfg = accentConfigs[index % accentConfigs.length];
  const { frontmatter, slug } = post;
  return (
    <motion.div initial={{ opacity: 0, y: 20 }} animate={{ opacity: 1, y: 0 }} transition={{ duration: 0.3, delay: index * 0.1 }}>
      <Link href={`/posts/${slug}`} className={`block rounded-card border p-5 mb-3.5 ${cfg.bg} ${cfg.border} ${cfg.glow} no-underline transition-all duration-300 hover:scale-[1.01] hover:shadow-[0_4px_20px_rgba(120,140,100,0.08)]`}>
        <div className="flex items-center gap-4">
          <div className={`w-10 h-10 rounded-[10px] border ${cfg.border} ${cfg.bg} flex items-center justify-center shrink-0`}>
            <span className={`font-mono text-lg ${cfg.iconColor}`}>{cfg.icon}</span>
          </div>
          <div className="flex-1 min-w-0">
            <h2 className="text-[15px] font-semibold text-olive mb-1 leading-snug">{frontmatter.title}</h2>
            <div className="flex items-center gap-1.5 flex-wrap text-[10px]">
              <span className={cfg.dateColor}>{frontmatter.date}</span>
              <span className="text-olive-light/15">·</span>
              {frontmatter.tags.map((tag) => (<span key={tag} className={`px-2 py-0.5 rounded-tag font-medium ${cfg.tagBg} ${cfg.tagColor}`}>#{tag}</span>))}
            </div>
          </div>
          <span className={`font-mono text-sm shrink-0 ${cfg.arrowColor}`}>→</span>
        </div>
      </Link>
    </motion.div>
  );
}
