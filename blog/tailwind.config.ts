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
  plugins: [require("@tailwindcss/typography")],
};

export default config;
