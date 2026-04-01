# mdBook Template

A book template powered by [mdBook](https://rust-lang.github.io/mdBook/).

## Quick Start

```bash
git clone -b mdbook git@github.com:qiekn/docs.git my-book
cd my-book
mdbook serve --open
```

## Install mdBook

```bash
cargo install mdbook
```

Or download a prebuilt binary from [GitHub Releases](https://github.com/rust-lang/mdBook/releases).

## Structure

```
.
├── book.toml
├── src/
│   ├── SUMMARY.md
│   └── chapter_1.md
├── .gitignore
└── README.md
```

## Configuration

Edit `book.toml` to customize your book. See the [mdBook documentation](https://rust-lang.github.io/mdBook/format/configuration/) for full reference.
