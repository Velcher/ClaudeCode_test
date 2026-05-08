"use client";

interface Props { title: string; description?: string; }

export default function VisualDemo({ title, description }: Props) {
  return (
    <div className="rounded-card border border-sage/10 p-4 mb-4 flex items-center gap-3 bg-sage-light/20">
      <div className="w-9 h-9 rounded-lg bg-sage/10 flex items-center justify-center font-mono text-lg text-sage">▶</div>
      <div>
        <div className="font-semibold text-[13px] text-olive">{title}</div>
        {description && <div className="text-[10px] text-olive-light/50 mt-0.5">{description}</div>}
      </div>
    </div>
  );
}
