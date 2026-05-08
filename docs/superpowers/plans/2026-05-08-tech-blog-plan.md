# Tech Blog Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a personal tech blog with Next.js App Router, MDX content, warm eye-friendly design, and subtle animations.

**Architecture:** File-driven MDX blog with Next.js App Router. Content lives in `content/` as `.md` files, parsed at build time via `gray-matter` + `next-mdx-remote`. Tailwind CSS for styling, Framer Motion for animations. Three alternating accent colors per post card.

**Tech Stack:** Next.js 14, React 18, TypeScript, Tailwind CSS 3, Framer Motion, next-mdx-remote, gray-matter

---

### Task 1: Scaffold Next.js Project

**Files:**
- Create: `package.json`, `tsconfig.json`, `next.config.mjs`, `tailwind.config.ts`, `postcss.config.mjs`, `src/app/globals.css`

- [ ] **Step 1: Create project directory structure and package.json**

```bash
mkdir -p src/app src/app/posts/\[slug\] src/app/about src/components src/lib src/content
```

Create `package.json`:
```json
{
  "name": "fmopa-blog",
  "version": "0.1.0",
  "private": true,
  "scripts": {
    "dev": "next dev",
    "build": "next build",
    "start": "next start"
  },
  "dependencies": {
    "next": "^14.2.0",
    "react": "^18.3.0",
    "react-dom": "^18.3.0",
    "next-mdx-remote": "^4.4.1",
    "gray-matter": "^4.0.3",
    "framer-motion": "^11.0.0",
    "rehype-highlight": "^7.0.0",
    "rehype-slug": "^6.0.0"
  },
  "devDependencies": {
    "typescript": "^5.4.0",
    "@types/node": "^20.0.0",
    "@types/react": "^18.3.0",
    "@types/react-dom": "^18.3.0",
    "tailwindcss": "^3.4.0",
    "postcss": "^8.4.0",
    "autoprefixer": "^10.4.0"
  }
}
```

- [ ] **Step 2: Run npm install**

```bash
npm install
```

- [ ] **Step 3: Create tsconfig.json**

```json
{
  "compilerOptions": {
    "target": "ES2017",
    "lib": ["dom", "dom.iterable", "esnext"],
    "allowJs": true,
    "skipLibCheck": true,
    "strict": true,
    "noEmit": true,
    "esModuleInterop": true,
    "module": "esnext",
    "moduleResolution": "bundler",
    "resolveJsonModule": true,
    "isolatedModules": true,
    "jsx": "preserve",
    "incremental": true,
    "plugins": [{ "name": "next" }],
    "paths": { "@/*": ["./src/*"] }
  },
  "include": ["next-env.d.ts", "**/*.ts", "**/*.tsx", ".next/types/**/*.ts"],
  "exclude": ["node_modules"]
}
```

- [ ] **Step 4: Create next.config.mjs**

```javascript
/** @type {import('next').NextConfig} */
const nextConfig = {};

export default nextConfig;
```

- [ ] **Step 5: Create Tailwind + PostCSS configs**

Create `tailwind.config.ts`:
```typescript
import type { Config } from "tailwindcss";

const config: Config = {
  content: ["./src/**/*.{ts,tsx}"],
  theme: {
    extend: {
      colors: {
        paper: "#faf7f0",
        "paper-dark": "#f6f3ea",
        sage: {
          light: "#e8efe0",
          DEFAULT: "#7a9a6a",
          dark: "#6b8b5c",
        },
        amber: {
          light: "#f5ecd8",
          DEFAULT: "#b8944c",
        },
        teal: {
          light: "#dce8ec",
          DEFAULT: "#5c8a94",
        },
        olive: {
          DEFAULT: "#3d4a35",
          light: "#5a6048",
        },
      },
      fontFamily: {
        sans: ['"Noto Sans SC"', "sans-serif"],
        mono: ['"JetBrains Mono"', "monospace"],
      },
      borderRadius: {
        card: "12px",
        tag: "6px",
      },
    },
  },
  plugins: [],
};

export default config;
```

Create `postcss.config.mjs`:
```javascript
/** @type {import('postcss-load-config').Config} */
const config = {
  plugins: {
    tailwindcss: {},
    autoprefixer: {},
  },
};

export default config;
```

- [ ] **Step 6: Create globals.css**

Create `src/app/globals.css`:
```css
@tailwind base;
@tailwind components;
@tailwind utilities;

@import url('https://fonts.googleapis.com/css2?family=Noto+Sans+SC:wght@300;400;500;600;700&family=JetBrains+Mono:wght@400;600&display=swap');

@layer base {
  body {
    @apply bg-paper text-olive-light font-sans;
  }
}
```

- [ ] **Step 7: Commit**

```bash
git add package.json package-lock.json tsconfig.json next.config.mjs tailwind.config.ts postcss.config.mjs src/app/globals.css
git commit -m "feat: scaffold Next.js project with Tailwind config"
```

---

### Task 2: Create lib/posts.ts — MDX Content Parser

**Files:**
- Create: `src/lib/posts.ts`

- [ ] **Step 1: Write posts.ts**

```typescript
import fs from "fs";
import path from "path";
import matter from "gray-matter";
import { compileMDX } from "next-mdx-remote/rsc";
import rehypeHighlight from "rehype-highlight";
import rehypeSlug from "rehype-slug";

const contentDir = path.join(process.cwd(), "src", "content");

export interface PostFrontmatter {
  title: string;
  date: string;
  tags: string[];
  excerpt?: string;
}

export interface Post {
  slug: string;
  frontmatter: PostFrontmatter;
  rawContent: string;
}

export function getAllPosts(): Post[] {
  if (!fs.existsSync(contentDir)) return [];

  const files = fs.readdirSync(contentDir).filter((f) => f.endsWith(".md"));

  const posts = files
    .map((filename) => {
      const filePath = path.join(contentDir, filename);
      const raw = fs.readFileSync(filePath, "utf-8");
      const { data } = matter(raw);
      const slug = filename.replace(/^\d{4}-\d{2}-\d{2}-/, "").replace(/\.md$/, "");

      return {
        slug,
        frontmatter: {
          title: data.title || slug,
          date: data.date || "",
          tags: data.tags || [],
          excerpt: data.excerpt || "",
        },
        rawContent: raw,
      };
    })
    .sort(
      (a, b) =>
        new Date(b.frontmatter.date).getTime() -
        new Date(a.frontmatter.date).getTime()
    );

  return posts;
}

export function getPostBySlug(slug: string): Post | null {
  const posts = getAllPosts();
  return posts.find((p) => p.slug === slug) || null;
}

export async function compilePostContent(rawContent: string) {
  const { content } = await compileMDX({
    source: rawContent,
    options: {
      mdxOptions: {
        rehypePlugins: [rehypeHighlight, rehypeSlug],
      },
      parseFrontmatter: true,
    },
  });

  return content;
}
```

- [ ] **Step 2: Verify TypeScript compiles**

```bash
npx tsc --noEmit
```
Expected: no errors related to `src/lib/posts.ts`.

- [ ] **Step 3: Commit**

```bash
git add src/lib/posts.ts
git commit -m "feat: add MDX content parser"
```

---

### Task 3: Root Layout with Fonts + Metadata

**Files:**
- Create: `src/app/layout.tsx`

- [ ] **Step 1: Write layout.tsx**

```typescript
import type { Metadata } from "next";
import "./globals.css";

export const metadata: Metadata = {
  title: {
    default: "FMoPA.Blog — 深入底层计算",
    template: "%s | FMoPA.Blog",
  },
  description: "ARM SME · FMoPA 指令 · 矩阵乘法 · 性能调优",
};

export default function RootLayout({
  children,
}: {
  children: React.ReactNode;
}) {
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
      <div className="max-w-3xl mx-auto px-6 h-14 flex items-center justify-between">
        <a
          href="/"
          className="font-mono font-semibold text-sage-dark no-underline text-lg"
        >
          FMoPA.Blog
        </a>
        <div className="flex gap-6 text-sm font-medium">
          <a href="/" className="text-sage no-underline">
            Posts
          </a>
          <a href="/about" className="text-olive-light/50 no-underline hover:text-olive-light/70 transition-colors">
            About
          </a>
        </div>
      </div>
    </nav>
  );
}

function Footer() {
  return (
    <footer className="border-t border-amber/10 bg-paper-dark/50">
      <div className="max-w-3xl mx-auto px-6 h-12 flex items-center justify-between text-[10px] text-olive-light/40">
        <span>&copy; 2026 FMoPA.Blog</span>
        <span className="font-mono">sys.online</span>
        <span>Built with Next.js</span>
      </div>
    </footer>
  );
}
```

- [ ] **Step 2: Verify build**

```bash
npx tsc --noEmit
```
Expected: no errors.

- [ ] **Step 3: Commit**

```bash
git add src/app/layout.tsx
git commit -m "feat: add root layout with nav and footer"
```

---

### Task 4: PostCard Component

**Files:**
- Create: `src/components/PostCard.tsx`

- [ ] **Step 1: Write PostCard.tsx**

```typescript
import Link from "next/link";
import { Post } from "@/lib/posts";

const accentConfigs = [
  {
    icon: "▶",
    bg: "bg-sage-light/30",
    border: "border-sage/15",
    iconColor: "text-sage",
    dateColor: "text-sage/50",
    tagBg: "bg-sage/10",
    tagColor: "text-sage-dark",
    arrowColor: "text-sage/30",
    glow: "shadow-[0_2px_12px_rgba(100,120,80,0.04)]",
  },
  {
    icon: "◆",
    bg: "bg-amber-light/30",
    border: "border-amber/15",
    iconColor: "text-amber",
    dateColor: "text-amber/50",
    tagBg: "bg-amber/10",
    tagColor: "text-amber",
    arrowColor: "text-amber/30",
    glow: "shadow-[0_2px_12px_rgba(120,100,60,0.04)]",
  },
  {
    icon: "◇",
    bg: "bg-teal-light/30",
    border: "border-teal/15",
    iconColor: "text-teal",
    dateColor: "text-teal/50",
    tagBg: "bg-teal/10",
    tagColor: "text-teal",
    arrowColor: "text-teal/30",
    glow: "shadow-[0_2px_12px_rgba(80,110,120,0.04)]",
  },
];

interface Props {
  post: Post;
  index: number;
}

export default function PostCard({ post, index }: Props) {
  const cfg = accentConfigs[index % accentConfigs.length];
  const { frontmatter, slug } = post;

  return (
    <Link
      href={`/posts/${slug}`}
      className={`block rounded-card border p-5 mb-3.5 ${cfg.bg} ${cfg.border} ${cfg.glow} no-underline transition-all duration-300 hover:scale-[1.01]`}
    >
      <div className="flex items-center gap-4">
        <div className={`w-10 h-10 rounded-[10px] border ${cfg.border} ${cfg.bg} flex items-center justify-center shrink-0`}>
          <span className={`font-mono text-lg ${cfg.iconColor}`}>{cfg.icon}</span>
        </div>
        <div className="flex-1 min-w-0">
          <h2 className="text-[15px] font-semibold text-olive mb-1 leading-snug">
            {frontmatter.title}
          </h2>
          <div className="flex items-center gap-1.5 flex-wrap text-[10px]">
            <span className={cfg.dateColor}>{frontmatter.date}</span>
            <span className="text-olive-light/15">·</span>
            {frontmatter.tags.map((tag) => (
              <span
                key={tag}
                className={`px-2 py-0.5 rounded-tag font-medium ${cfg.tagBg} ${cfg.tagColor}`}
              >
                #{tag}
              </span>
            ))}
          </div>
        </div>
        <span className={`font-mono text-sm shrink-0 ${cfg.arrowColor}`}>→</span>
      </div>
    </Link>
  );
}
```

- [ ] **Step 2: Commit**

```bash
git add src/components/PostCard.tsx
git commit -m "feat: add PostCard component with alternating accent colors"
```

---

### Task 5: Homepage

**Files:**
- Create: `src/app/page.tsx`

- [ ] **Step 1: Write page.tsx**

```typescript
import { getAllPosts } from "@/lib/posts";
import PostCard from "@/components/PostCard";

export default function HomePage() {
  const posts = getAllPosts();

  return (
    <>
      <section className="bg-gradient-to-br from-[#f0ede0] via-[#f8f4ea] to-[#f2efe0] border-b border-amber/10">
        <div className="max-w-3xl mx-auto px-6 py-12">
          <p className="font-mono text-[10px] text-sage/60 tracking-[3px] mb-3.5">
            &gt; WELCOME_TO_THE_BLOG_
          </p>
          <h1 className="text-[26px] font-bold text-olive mb-2 leading-snug tracking-wide">
            深入底层计算的
            <br />
            奇妙世界
          </h1>
          <p className="text-[13px] text-olive-light/60 mb-3.5 leading-relaxed">
            ARM SME · FMoPA 指令 · 矩阵乘法 · 性能调优
          </p>
          <p className="text-xs text-amber font-normal italic">
            每一行代码，都在理解一颗芯片的呼吸
          </p>
        </div>
      </section>

      <section className="max-w-3xl mx-auto px-6 py-8">
        <p className="font-mono text-[10px] text-sage/50 tracking-[3px] mb-4 ml-1">
          &gt; LATEST_POSTS_
        </p>

        {posts.length === 0 ? (
          <p className="text-olive-light/40 text-sm py-12 text-center">
            还没有文章，在 src/content/ 目录下创建 .md 文件即可
          </p>
        ) : (
          posts.map((post, i) => (
            <PostCard key={post.slug} post={post} index={i} />
          ))
        )}
      </section>
    </>
  );
}
```

- [ ] **Step 2: Verify TypeScript**

```bash
npx tsc --noEmit
```
Expected: no errors.

- [ ] **Step 3: Commit**

```bash
git add src/app/page.tsx
git commit -m "feat: add homepage with hero and post list"
```

---

### Task 6: CodeBlock Component

**Files:**
- Create: `src/components/CodeBlock.tsx`

- [ ] **Step 1: Write CodeBlock.tsx**

```typescript
"use client";

import { useState } from "react";

interface Props {
  filename?: string;
  language?: string;
  children: string;
}

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
        <button
          onClick={handleCopy}
          className="text-sage/30 hover:text-sage/60 transition-colors bg-transparent border-none cursor-pointer font-mono text-[10px]"
        >
          {copied ? "copied!" : "copy"}
        </button>
      </div>
      <pre className="px-4 py-3 m-0 overflow-x-auto font-mono text-[11px] leading-relaxed text-olive/80">
        <code className={`language-${language || "text"}`}>{children}</code>
      </pre>
    </div>
  );
}
```

- [ ] **Step 2: Commit**

```bash
git add src/components/CodeBlock.tsx
git commit -m "feat: add CodeBlock component with copy button"
```

---

### Task 7: VisualDemo Placeholder + MDX Components

**Files:**
- Create: `src/components/VisualDemo.tsx`, `src/components/mdx-components.tsx`

- [ ] **Step 1: Write VisualDemo.tsx**

```typescript
"use client";

interface Props {
  title: string;
  description?: string;
}

export default function VisualDemo({ title, description }: Props) {
  return (
    <div className="rounded-card border border-sage/10 p-4 mb-4 flex items-center gap-3 bg-sage-light/20">
      <div className="w-9 h-9 rounded-lg bg-sage/10 flex items-center justify-center font-mono text-lg text-sage">
        ▶
      </div>
      <div>
        <div className="font-semibold text-[13px] text-olive">{title}</div>
        {description && (
          <div className="text-[10px] text-olive-light/50 mt-0.5">
            {description}
          </div>
        )}
      </div>
    </div>
  );
}
```

- [ ] **Step 2: Write mdx-components.tsx**

```typescript
import type { MDXComponents } from "mdx/types";
import CodeBlock from "./CodeBlock";

export function useMDXComponents(components: MDXComponents): MDXComponents {
  return {
    pre: ({ children, ...props }) => {
      const child = children as React.ReactElement | undefined;
      const codeProps = child?.props as Record<string, unknown> | undefined;
      return (
        <CodeBlock
          filename={codeProps?.["data-filename"] as string}
          language={codeProps?.["className"]?.toString().replace("language-", "")}
        >
          {codeProps?.children as string || ""}
        </CodeBlock>
      );
    },
    ...components,
  };
}
```

- [ ] **Step 3: Commit**

```bash
git add src/components/VisualDemo.tsx src/components/mdx-components.tsx
git commit -m "feat: add VisualDemo and MDX components mapping"
```

---

### Task 8: Article Detail Page

**Files:**
- Create: `src/app/posts/[slug]/page.tsx`

- [ ] **Step 1: Write [slug]/page.tsx**

```typescript
import { notFound } from "next/navigation";
import Link from "next/link";
import { getPostBySlug, getAllPosts, compilePostContent } from "@/lib/posts";
import "highlight.js/styles/github.css";

interface Props {
  params: { slug: string };
}

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
    <article className="max-w-[760px] mx-auto px-6 py-8">
      {/* Progress bar */}
      <div className="h-0.5 bg-olive/5 rounded-full mb-8">
        <div className="h-full w-[35%] bg-gradient-to-r from-sage to-teal rounded-full" />
      </div>

      {/* Header */}
      <header className="mb-6">
        <p className="font-mono text-[10px] text-sage/50 tracking-[3px] mb-2">
          &gt; POST_DETAIL_
        </p>
        <h1 className="text-2xl font-bold text-olive mb-2.5 leading-snug">
          {post.frontmatter.title}
        </h1>
        <div className="flex items-center gap-2.5 text-[11px] flex-wrap">
          <span className="text-sage/70">{post.frontmatter.date}</span>
          <span className="text-olive-light/10">|</span>
          <span className="text-olive-light/40">12 min read</span>
          {post.frontmatter.tags.map((tag, i) => {
            const cfg = [
              "bg-sage/10 text-sage-dark",
              "bg-amber/10 text-amber",
              "bg-teal/10 text-teal",
            ][i % 3];
            return (
              <span key={tag} className={`px-2 py-0.5 rounded-tag text-[9px] font-medium ${cfg}`}>
                #{tag}
              </span>
            );
          })}
        </div>
      </header>

      {/* Content */}
      <div className="prose prose-olive max-w-none leading-relaxed text-[14px] text-olive-light/80 [&_p]:font-light">
        {content}
      </div>

      {/* Bottom navigation */}
      <div className="border-t border-olive/5 mt-8 pt-4 flex justify-between items-center text-[11px]">
        {prevPost ? (
          <Link href={`/posts/${prevPost.slug}`} className="text-sage/40 no-underline hover:text-sage/70 transition-colors">
            &lt; {prevPost.frontmatter.title}
          </Link>
        ) : <span />}
        <div className="flex gap-2">
          <Link href="/" className="px-3 py-1.5 rounded-lg bg-sage/5 border border-sage/10 text-sage/60 text-[10px] no-underline hover:bg-sage/10 transition-colors">
            返回首页
          </Link>
        </div>
        {nextPost ? (
          <Link href={`/posts/${nextPost.slug}`} className="text-sage/40 no-underline hover:text-sage/70 transition-colors">
            {nextPost.frontmatter.title} &gt;
          </Link>
        ) : <span />}
      </div>
    </article>
  );
}
```

- [ ] **Step 2: Verify TypeScript**

```bash
npx tsc --noEmit
```
Expected: no errors.

- [ ] **Step 3: Commit**

```bash
git add src/app/posts/[slug]/page.tsx
git commit -m "feat: add article detail page with MDX rendering"
```

---

### Task 9: About Page

**Files:**
- Create: `src/app/about/page.tsx`

- [ ] **Step 1: Write about/page.tsx**

```typescript
import type { Metadata } from "next";

export const metadata: Metadata = {
  title: "About",
};

const skills = [
  "C",
  "ARM Assembly",
  "SME",
  "FMoPA",
  "Verilog",
  "矩阵计算",
  "性能调优",
  "嵌入式开发",
];

export default function AboutPage() {
  return (
    <div className="max-w-[640px] mx-auto px-6 py-12">
      <p className="font-mono text-[10px] text-sage/50 tracking-[3px] mb-4">
        &gt; ABOUT_ME_
      </p>

      <h1 className="text-2xl font-bold text-olive mb-6">关于我</h1>

      <div className="rounded-card border border-sage/10 bg-paper-dark/50 p-6 mb-6">
        <p className="text-[14px] text-olive-light/70 leading-relaxed font-light">
          底层计算爱好者，专注于 ARM 架构与矩阵计算。
          喜欢深挖指令集背后的硬件设计哲学，
          用代码理解每一颗芯片的呼吸。
        </p>
      </div>

      <h2 className="text-lg font-semibold text-olive mb-3">技术栈</h2>
      <div className="flex flex-wrap gap-2 mb-8">
        {skills.map((skill, i) => {
          const cfg = [
            "bg-sage/10 text-sage-dark border-sage/15",
            "bg-amber/10 text-amber border-amber/15",
            "bg-teal/10 text-teal border-teal/15",
          ][i % 3];
          return (
            <span
              key={skill}
              className={`px-3 py-1 rounded-tag text-xs font-medium border ${cfg}`}
            >
              {skill}
            </span>
          );
        })}
      </div>

      <h2 className="text-lg font-semibold text-olive mb-3">链接</h2>
      <div className="flex gap-4">
        <a
          href="https://github.com/Velcher"
          target="_blank"
          rel="noopener noreferrer"
          className="text-sage/60 text-sm no-underline hover:text-sage transition-colors"
        >
          GitHub
        </a>
        <span className="text-olive-light/15">·</span>
        <span className="text-sage/60 text-sm">
          {/* email placeholder */}
        </span>
      </div>
    </div>
  );
}
```

- [ ] **Step 2: Commit**

```bash
git add src/app/about/page.tsx
git commit -m "feat: add about page"
```

---

### Task 10: Custom 404 Page

**Files:**
- Create: `src/app/not-found.tsx`

- [ ] **Step 1: Write not-found.tsx**

```typescript
import Link from "next/link";

export default function NotFound() {
  return (
    <div className="max-w-[480px] mx-auto px-6 py-20 text-center">
      <p className="font-mono text-6xl text-sage/15 mb-4">404</p>
      <p className="font-mono text-[10px] text-sage/40 tracking-[3px] mb-3">
        &gt; SIGNAL_LOST_
      </p>
      <h1 className="text-xl font-semibold text-olive mb-2">信号丢失</h1>
      <p className="text-[13px] text-olive-light/50 mb-6 font-light">
        这个页面不存在，可能已被移除或链接失效
      </p>
      <Link
        href="/"
        className="inline-block px-4 py-2 rounded-lg bg-sage/10 border border-sage/15 text-sage-dark text-sm no-underline hover:bg-sage/15 transition-colors"
      >
        返回首页
      </Link>
    </div>
  );
}
```

- [ ] **Step 2: Commit**

```bash
git add src/app/not-found.tsx
git commit -m "feat: add custom 404 page"
```

---

### Task 11: Sample Blog Post

**Files:**
- Create: `src/content/2026-05-08-sme-fmopa.md`

- [ ] **Step 1: Write sample post**

Create `src/content/2026-05-08-sme-fmopa.md`:
```markdown
---
title: "SME 矩阵乘法：FMoPA 指令深度解析"
date: "2026-05-08"
tags: ["matrix", "sme", "fmopa"]
excerpt: "深入讲解 ARM SME 扩展中 FMoPA 指令的工作流程..."
---

在现代 ARM 架构中，**SME**（Scalable Matrix Extension）提供了一套全新的矩阵计算能力。本文将深入讲解 FMoPA 指令的完整工作流程。

## 什么是 FMoPA

FMoPA（Floating-point Matrix Outer Product and Accumulate）是 SME 扩展中的核心指令。它的关键操作是**外层积**（Outer Product）——一次性计算向量 × 向量的完整结果矩阵。

```c
// FMoPA: 外层积矩阵乘法
void sme_matmul_fmopa(
    const float *A,
    const float *B,
    float *C,
    int M, int N, int K
) {
    smstart();
    // 分块计算...
    // 每个 FMoPA 指令处理一整块 ZA 寄存器
    smstop();
}
```

## 为什么外层积比点积快

传统矩阵乘法使用点积计算每个输出元素：

```
C[i][j] = Σ A[i][k] * B[k][j]
```

每个 C[i][j] 需要一次完整的归约循环。而外层积一次性把整行/整列的乘积展开到输出矩阵中，大大减少了指令开销。

## 总结

FMoPA 指令通过外层积的并行化策略，在 ARM 处理器上实现了高效的矩阵乘法计算。
```

- [ ] **Step 2: Commit**

```bash
git add src/content/2026-05-08-sme-fmopa.md
git commit -m "feat: add sample blog post"
```

---

### Task 12: Add Framer Motion Animations

**Files:**
- Modify: `src/app/page.tsx`, `src/app/posts/[slug]/page.tsx`, `src/components/PostCard.tsx`

- [ ] **Step 1: Add page transition animation wrapper**

Create `src/components/PageTransition.tsx`:
```typescript
"use client";

import { motion } from "framer-motion";

export default function PageTransition({
  children,
}: {
  children: React.ReactNode;
}) {
  return (
    <motion.div
      initial={{ opacity: 0, y: 12 }}
      animate={{ opacity: 1, y: 0 }}
      transition={{ duration: 0.4, ease: "easeOut" }}
    >
      {children}
    </motion.div>
  );
}
```

- [ ] **Step 2: Add hover animation to PostCard**

Update `src/components/PostCard.tsx`:

Change the outer `<Link>` wrapper from a plain `<Link>` to using `motion.a`:

```typescript
"use client";

import { motion } from "framer-motion";
import Link from "next/link";
import { Post } from "@/lib/posts";

const accentConfigs = [
  // ... (same as before, unchanged)
];

interface Props {
  post: Post;
  index: number;
}

export default function PostCard({ post, index }: Props) {
  const cfg = accentConfigs[index % accentConfigs.length];
  const { frontmatter, slug } = post;

  return (
    <motion.div
      initial={{ opacity: 0, y: 20 }}
      animate={{ opacity: 1, y: 0 }}
      transition={{ duration: 0.3, delay: index * 0.1 }}
    >
      <Link
        href={`/posts/${slug}`}
        className={`block rounded-card border p-5 mb-3.5 ${cfg.bg} ${cfg.border} ${cfg.glow} no-underline transition-all duration-300 hover:scale-[1.01] hover:shadow-[0_4px_20px_rgba(120,140,100,0.08)]`}
      >
        {/* Inner content unchanged */}
      </Link>
    </motion.div>
  );
}
```

- [ ] **Step 3: Wrap pages with PageTransition**

In `src/app/page.tsx`, wrap the return content:
```typescript
import PageTransition from "@/components/PageTransition";
// ...
export default function HomePage() {
  // ... same logic
  return (
    <PageTransition>
      {/* existing content unchanged */}
    </PageTransition>
  );
}
```

In `src/app/posts/[slug]/page.tsx`, wrap the return content:
```typescript
import PageTransition from "@/components/PageTransition";
// ...
export default async function PostPage({ params }: Props) {
  // ... same logic
  return (
    <PageTransition>
      <article className="max-w-[760px] mx-auto px-6 py-8">
        {/* existing content unchanged */}
      </article>
    </PageTransition>
  );
}
```

In `src/app/about/page.tsx`, wrap the return content with `<PageTransition>` similarly.

- [ ] **Step 4: Verify build**

```bash
npx tsc --noEmit
```
Expected: no errors.

- [ ] **Step 5: Commit**

```bash
git add src/components/PageTransition.tsx src/components/PostCard.tsx src/app/page.tsx src/app/posts/[slug]/page.tsx src/app/about/page.tsx
git commit -m "feat: add Framer Motion page transitions and card animations"
```

---

### Task 13: Final Verification

- [ ] **Step 1: Run full build**

```bash
npm run build
```
Expected: successful build with no errors.

- [ ] **Step 2: Run dev server and verify pages**

```bash
npm run dev
```

Check the following URLs:
- `http://localhost:3000/` — Homepage with hero, nav, post list, footer
- `http://localhost:3000/posts/sme-fmopa` — Sample article with code block
- `http://localhost:3000/about` — About page
- `http://localhost:3000/nonexistent` — Custom 404 page

- [ ] **Step 3: Add .gitignore for build artifacts**

Ensure `.gitignore` includes:
```
node_modules/
.next/
out/
.env
.env.local
.superpowers/
```

- [ ] **Step 4: Final commit**

```bash
git add .gitignore
git commit -m "chore: add .gitignore for build artifacts"
```

---

### Task 14: Push to GitHub

- [ ] **Step 1: Push all commits**

```bash
git push
```
