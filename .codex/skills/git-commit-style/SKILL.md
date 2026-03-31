---
name: git-commit-style
description: Write git commit messages that match this repository owner's style. Use when preparing a commit, suggesting a commit message, reviewing commit wording, or summarizing a code change into a subject line for this project.
---

# Git Commit Style

## Overview

Infer the preferred commit style from local repository history, then write a short subject line that matches it. Default to the established pattern already visible in this repo unless the user asks for a different convention.

## Workflow

1. Inspect recent local commit subjects before proposing wording.
2. Prefer the dominant pattern over generic best practices.
3. If local history is sparse, state that briefly and extrapolate conservatively.

## Style Rules For This Repo

Current local evidence is limited, but the visible sample uses:

- A lowercase conventional prefix such as `chore:`
- A short imperative English subject
- Optional context in parentheses, for example `chore: update run.sh (using cmake and ninja)`

Follow these defaults:

- Format: `<type>: <short action>`
- Keep the subject compact and specific
- Use lowercase after the prefix unless a proper noun requires otherwise
- Add parentheses only when the extra context clarifies a tool, migration, or build detail
- Avoid trailing periods

## Choosing The Prefix

Use the smallest accurate prefix:

- `fix:` for bug fixes and regressions
- `feat:` for user-facing or gameplay features
- `refactor:` for code reshaping without behavior change
- `chore:` for build, tooling, dependency, or housekeeping updates
- `docs:` for documentation-only changes

## Fallback Behavior

If history remains too thin to establish a stronger pattern, keep using the same concise conventional-commit style rather than inventing repo-specific flourishes. When the user asks for a commit message, return one or two strong candidates, not a long explanation.
