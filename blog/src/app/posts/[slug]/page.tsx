import { notFound } from "next/navigation";
import Link from "next/link";
import { getPostBySlug, getAllPosts, compilePostContent } from "@/lib/posts";
import PageTransition from "@/components/PageTransition";
import "highlight.js/styles/github.css";

interface Props { params: { slug: string }; }

export function generateStaticParams() {
  return getAllPosts().map((post) => ({ slug: post.slug }));
}

export async function generateMetadata({ params }: Props) {
  const post = getPostBySlug(params.slug);
  if (!post) return { title: "Not Found" };
  return { title: post.frontmatter.title };
}

export default async function PostPage({ params }: Props) {
  const post = getPostBySlug(params.slug);
  if (!post) notFound();
  const content = await compilePostContent(post.rawContent);
  const allPosts = getAllPosts();
  const currentIndex = allPosts.findIndex((p) => p.slug === params.slug);
  const prevPost = allPosts[currentIndex + 1] || null;
  const nextPost = allPosts[currentIndex - 1] || null;

  return (
    <PageTransition>
    <article className="max-w-[760px] mx-auto px-4 sm:px-6 py-6 sm:py-8">
      <div className="h-0.5 bg-olive/5 rounded-full mb-6 sm:mb-8">
        <div className="h-full w-[35%] bg-gradient-to-r from-sage to-teal rounded-full" />
      </div>
      <header className="mb-6">
        <p className="font-mono text-[9px] sm:text-[10px] text-sage/50 tracking-[3px] mb-2">&gt; POST_DETAIL_</p>
        <h1 className="text-xl sm:text-2xl font-bold text-olive mb-2.5 leading-snug">{post.frontmatter.title}</h1>
        <div className="flex items-center gap-2.5 text-[10px] sm:text-[11px] flex-wrap">
          <span className="text-sage/70">{post.frontmatter.date}</span>
          <span className="text-olive-light/10">|</span>
          <span className="text-olive-light/40">12 min read</span>
          {post.frontmatter.tags.map((tag, i) => {
            const cfg = ["bg-sage/10 text-sage-dark", "bg-amber/10 text-amber", "bg-teal/10 text-teal"][i % 3];
            return (<span key={tag} className={`px-2 py-0.5 rounded-tag text-[9px] font-medium ${cfg}`}>#{tag}</span>);
          })}
        </div>
      </header>
      <div className="prose prose-olive max-w-none leading-relaxed text-[13px] sm:text-[14px] text-olive-light/80 [&_p]:font-light">{content}</div>
      <nav className="border-t border-olive/5 mt-8 pt-4 grid grid-cols-[1fr_auto_1fr] items-center gap-2 text-[10px] sm:text-[11px]">
        {prevPost ? <Link href={`/posts/${prevPost.slug}`} className="text-sage/40 no-underline hover:text-sage/70 transition-colors truncate">&lt; {prevPost.frontmatter.title}</Link> : <span />}
        <Link href="/" className="px-2 sm:px-3 py-1.5 rounded-lg bg-sage/5 border border-sage/10 text-sage/60 text-[10px] no-underline hover:bg-sage/10 transition-colors whitespace-nowrap shrink-0">返回首页</Link>
        {nextPost ? <Link href={`/posts/${nextPost.slug}`} className="text-sage/40 no-underline hover:text-sage/70 transition-colors text-right truncate">{nextPost.frontmatter.title} &gt;</Link> : <span />}
      </nav>
    </article>
    </PageTransition>
  );
}
