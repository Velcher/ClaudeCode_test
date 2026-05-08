import { getAllPosts } from "@/lib/posts";
import PostCard from "@/components/PostCard";
import PageTransition from "@/components/PageTransition";

export default function HomePage() {
  const posts = getAllPosts();
  return (
    <PageTransition>
      <section className="bg-gradient-to-br from-[#f0ede0] via-[#f8f4ea] to-[#f2efe0] border-b border-amber/10">
        <div className="max-w-3xl mx-auto px-6 py-12">
          <p className="font-mono text-[10px] text-sage/60 tracking-[3px] mb-3.5">&gt; WELCOME_TO_THE_BLOG_</p>
          <h1 className="text-[26px] font-bold text-olive mb-2 leading-snug tracking-wide">深入底层计算的<br />奇妙世界</h1>
          <p className="text-[13px] text-olive-light/60 mb-3.5 leading-relaxed">ARM SME · FMoPA 指令 · 矩阵乘法 · 性能调优</p>
          <p className="text-xs text-amber font-normal italic">每一行代码，都在理解一颗芯片的呼吸</p>
        </div>
      </section>
      <section className="max-w-3xl mx-auto px-6 py-8">
        <p className="font-mono text-[10px] text-sage/50 tracking-[3px] mb-4 ml-1">&gt; LATEST_POSTS_</p>
        {posts.length === 0 ? (
          <p className="text-olive-light/40 text-sm py-12 text-center">还没有文章，在 src/content/ 目录下创建 .md 文件即可</p>
        ) : (
          posts.map((post, i) => (<PostCard key={post.slug} post={post} index={i} />))
        )}
      </section>
    </PageTransition>
  );
}
