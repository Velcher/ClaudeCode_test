# Personal Tech Blog — Design Spec

## Overview

A personal technical blog focused on ARM SME, FMoPA instructions, and matrix computation. React/Next.js with MDX for interactive articles.

## Tech Stack

- **Framework:** Next.js App Router (React 18+)
- **Content:** MDX + gray-matter + next-mdx-remote
- **Styling:** Tailwind CSS + Framer Motion (animations)
- **Deployment:** Vercel (auto-deploy on git push)

## Visual Design

### Color Palette (Warm Eye-Friendly)

| Role | Color | Hex |
|------|-------|-----|
| Background | Warm paper | `#faf7f0` |
| Primary | Sage green | `#7a9a6a` |
| Accent | Warm amber | `#b8944c` |
| Secondary | Mist teal | `#5c8a94` |
| Headings | Dark olive | `#3d4a35` |
| Body text | Semi-transparent dark green-gray | rgba |

### Typography
- **Body/Headings:** Noto Sans SC (humanist, readable)
- **Code/Terminal labels:** JetBrains Mono

### Design Principles
- Rounded corners on cards (10-12px), tags (6px)
- Glassmorphism navbar (backdrop-blur + semi-transparent bg)
- Subtle gradients in Hero section
- Three alternating accent colors for post cards (sage, amber, teal)
- Reserved animation hooks: scanlines, particles, typewriter, hover glow

## Architecture

```
src/
├── app/
│   ├── page.tsx                 # Homepage (post list + hero)
│   ├── posts/[slug]/page.tsx    # Article detail
│   ├── about/page.tsx           # About me
│   ├── not-found.tsx            # Custom 404
│   └── layout.tsx               # Root layout (nav + footer)
├── content/                     # Markdown articles (commit = publish)
│   └── YYYY-MM-DD-slug.md
├── components/
│   ├── Layout.tsx               # Navbar + Footer wrapper
│   ├── PostCard.tsx             # Article card (list)
│   ├── CodeBlock.tsx            # Syntax-highlighted code
│   └── VisualDemo.tsx           # Interactive MDX-embedded demo
└── lib/
    └── posts.ts                 # Read content/, parse MDX
```

## Pages

1. **Homepage (`/`)** — Hero + latest post list (classic layout, cards with alternating accent colors)
2. **Article Detail (`/posts/[slug]`)** — Reading progress bar, code blocks, embedded MDX demos, prev/next nav
3. **About (`/about`)** — Avatar, bio, tech stack tags, social links
4. **404** — "Signal lost" themed error page

## Data Flow

- `.md` files in `content/` → `lib/posts.ts` reads via `fs` + `gray-matter` → parses frontmatter + MDX body
- Homepage: lists all posts sorted by date
- Detail page: `generateStaticParams` pre-renders all posts (SSG)
- No database, no CMS — commit `.md` file, push, Vercel auto-deploys

## Error Handling

- 404 page for unknown routes
- Invalid slug → friendly "article not found" message
- MDX parse error → dev console error, production skips gracefully

## Dependencies

```json
{
  "next": "^14",
  "react": "^18",
  "typescript": "^5",
  "tailwindcss": "^3",
  "framer-motion": "^10",
  "next-mdx-remote": "^4",
  "gray-matter": "^4",
  "rehype-highlight": "^7",
  "rehype-slug": "^6"
}
```
