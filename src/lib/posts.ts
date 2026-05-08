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
    .sort((a, b) => new Date(b.frontmatter.date).getTime() - new Date(a.frontmatter.date).getTime());
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
      mdxOptions: { rehypePlugins: [rehypeHighlight as any, rehypeSlug] },
      parseFrontmatter: true,
    },
  });
  return content;
}
