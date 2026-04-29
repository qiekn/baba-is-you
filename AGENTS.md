read at ./CLAUDE.md

- Dont use cmake to build and test this project. Let me do this.
- Use UTF-8 and LF (don't use CRLF)

## SKILLS

### Git Commit Message

---
name: git-commit-message
description: Best practices for writing clear, structured, and meaningful git commit message.
---

#### Conventional Commits

```
type(scope): summary
```

| type     | usage                                      |
| -------- | --------------------                       |
| feat     | new user-visible feature                   |
| fix      | bug                                        |
| refactor | code restructuring without behavior change |
| perf     | performance improvement                    |
| chore    | maintenance / housekeeping                 |
| docs     | documentation changes                      |
| style    | formatting / style only                    |
| test     | tests                                      |

#### Core Principle

> A commit represents a **logical change (intent)**, not a sequence of actions.

#### When Generating a Commit Message

##### 1. Determine the intent

Classify the change:

* Is it user-visible? -> `feat`
* Is it fixing incorrect behavior? -> `fix`
* Is it restructuring code? -> `refactor`
* Is it improving performance? -> `perf`
* Is it just setup / assets / config? -> `chore`


##### 2. Infer the scope

Use the most relevant module:

Examples:

* texture
* bvh
* camera
* render
* scene
* material

---

##### 3. Write the summary

Rules:

* Use imperative mood
* Be concise but specific
* Avoid vague verbs like "update", "change", "fix stuff"

Good:

```
feat(texture): add Perlin noise with turbulence
perf(bvh): optimize traversal performance
refactor(camera): simplify ray generation logic
```

Bad:

```
feat: update texture
feat: improve things
feat: fix bug
```

---

#### Rewrite Rules (VERY IMPORTANT)

##### Rule 1: Collapse noisy commits

If multiple commits represent iterations of the same idea:

```
feat: tweak interpolation
feat: fix interpolation
feat: tweak again
```

-> Rewrite as:

```
perf(texture): improve interpolation stability
```

---

##### Rule 2: Avoid misusing `feat`

DO NOT use `feat` for:

* algorithm tweaks
* internal improvements
* refactoring
* experiments

---

##### Rule 3: Scene / demo changes

If change is not a core feature:

```
chore(scene): update Cornell box setup
```

---

##### Rule 4: Prefer meaning over chronology

DO NOT describe process:

```
feat: try new method
feat: fix previous mistake
```

```
refactor(texture): replace interpolation method
```

---

#### Granularity Rule

A good commit should:

* represent one idea
* be understandable in isolation
* be meaningful in project history

---

#### Output Format

Return ONLY the commit message:

```
type(scope): summary
```

No explanation unless explicitly requested.

---

#### Optional Extended Body (if needed)

```
type(scope): summary

- key change 1
- key change 2
- reason (optional)
```

---

#### Goal

Make git history readable as a **high-level evolution of the system**, not a log of developer actions.
