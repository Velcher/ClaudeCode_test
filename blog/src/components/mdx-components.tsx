import type { MDXComponents } from "mdx/types";
import CodeBlock from "./CodeBlock";

export function useMDXComponents(components: MDXComponents): MDXComponents {
  return {
    pre: ({ children }) => {
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
