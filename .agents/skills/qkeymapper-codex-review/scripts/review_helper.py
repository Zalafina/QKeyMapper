#!/usr/bin/env python3
"""Helper CLI for QKeyMapper local Gemini <-> Codex commit review workflows.

Strict non-elevated utility using Python standard library only. Never executes
git commit, git push, index-mutating optional locks, or elevated uac/gsudo commands.
All generated and checked review workspace artifacts live inside the repository root.
"""

from __future__ import annotations

import argparse
from datetime import datetime
import json
import os
from pathlib import Path, PureWindowsPath
import posixpath
import re
import stat
import subprocess
import sys
from typing import Any
import uuid


BATCH_RE = re.compile(r"^[A-Za-z0-9][A-Za-z0-9_.-]{0,127}$")
ROUND_FILE_RE = re.compile(
    r"^(\d{2})_(gemini|codex)_(submission|review|response|recheck)(?:_r(\d+))?\.md$"
)
FENCE_RE = re.compile(r"^\s*(`{3,}|~{3,})")
INLINE_CODE_RE = re.compile(r"`+[^`]*`+")
MD_LINK_RE = re.compile(r"\[[^\]]*\]\(([^)]*)\)")
LINE_FRAG_RE = re.compile(r"^L(\d+)(?:-L(\d+))?$")
HEX_SHA_RE = re.compile(r"^[0-9a-fA-F]{7,40}$")
HEX_RANGE_RE = re.compile(r"^`([0-9a-fA-F]{7,40})\.\.([0-9a-fA-F]{7,40})`")
WEB_SCHEME_RE = re.compile(r"^(?:https?://|mailto:)", re.IGNORECASE)
ANY_SCHEME_RE = re.compile(r"^[A-Za-z][A-Za-z0-9+.-]*:")
TABLE_KV_RE = re.compile(r"^\|\s*`([a-z0-9_]+)`\s*\|\s*(.*?)\s*\|\s*$")
REMOTE_META_RE = re.compile(r"^`([^`]+)`\s*@\s*`([0-9a-fA-F]{7,40})`$")

FILE_ATTRIBUTE_REPARSE_POINT = getattr(stat, "FILE_ATTRIBUTE_REPARSE_POINT", 0x400)

POST_ACTIONS = ("pause_for_user", "fix_only_then_pause", "continue_after_pass")
PROFILES = ("general", "handoff-plan")
REQUIRED_SUBMISSION_KEYS = (
    "branch",
    "remote_tracking",
    "deferred_commits",
    "review_range",
    "code_head",
    "doc_head",
    "post_review_action",
    "review_profile",
)


class HelperError(Exception):
    """Domain error with a concise user-facing message."""


def is_reparse_or_symlink(st: os.stat_result) -> bool:
    """Return True if lstat result represents a symlink or Windows reparse point."""
    attrs = getattr(st, "st_file_attributes", 0)
    return bool(stat.S_ISLNK(st.st_mode) or (attrs & FILE_ATTRIBUTE_REPARSE_POINT))


def find_repo_root(start: Path | None = None) -> Path:
    """Resolve the repository root containing .git from CWD or explicit start."""
    cur = (start or Path.cwd()).resolve()
    for candidate in (cur, *cur.parents):
        if (candidate / ".git").exists():
            return candidate
    raise HelperError(f"Could not locate Git repository root from {cur}")


def ensure_within_resolved(base_real: Path, target: Path) -> Path:
    """Verify target.resolve() stays within already-resolved base directory."""
    target_real = target.resolve()
    try:
        target_real.relative_to(base_real)
    except ValueError as exc:
        raise HelperError(f"Refusing path outside {base_real}: {target_real}") from exc
    return target_real


def validate_safe_rel_part(part: str) -> None:
    """Reject traversal, separators, empty segments, or NUL in single segment."""
    if not part or part in (".", "..") or "/" in part or "\\" in part or "\x00" in part:
        raise HelperError(f"Unsafe path segment rejected: {part!r}")


def validate_safe_dir_chain(
    repo_root: Path, rel_parts: tuple[str, ...], *, create_missing: bool = False
) -> Path:
    """Walk each segment from repo_root verifying real non-reparse directories."""
    root_real = repo_root.resolve()
    if not repo_root.is_dir():
        raise HelperError(f"Repository root is not a directory: {repo_root}")
    cur = repo_root
    for part in rel_parts:
        validate_safe_rel_part(part)
        cur = cur / part
        try:
            st = os.lstat(cur)
        except FileNotFoundError as exc:
            if not create_missing:
                raise HelperError(f"Missing required directory: {cur}") from exc
            cur.mkdir(exist_ok=False)
            st = os.lstat(cur)
        if is_reparse_or_symlink(st):
            raise HelperError(f"Refusing symlink or reparse directory: {cur}")
        if not stat.S_ISDIR(st.st_mode):
            raise HelperError(f"Expected directory, found non-directory: {cur}")
    ensure_within_resolved(root_real, cur)
    return cur


def validate_safe_existing_file(
    repo_root: Path,
    rel_parts: tuple[str, ...],
    *,
    reject_hardlinks: bool = False,
) -> Path:
    """Verify parent dir chain and target regular file with no symlink/reparse."""
    if not rel_parts:
        raise HelperError("Empty file path.")
    parent = validate_safe_dir_chain(repo_root, rel_parts[:-1], create_missing=False)
    validate_safe_rel_part(rel_parts[-1])
    target = parent / rel_parts[-1]
    try:
        st = os.lstat(target)
    except FileNotFoundError as exc:
        raise HelperError(f"Missing target file: {target}") from exc
    if is_reparse_or_symlink(st):
        raise HelperError(f"Refusing symlink or reparse file: {target}")
    if not stat.S_ISREG(st.st_mode):
        raise HelperError(f"Refusing non-regular file: {target}")
    if reject_hardlinks and st.st_nlink > 1:
        raise HelperError(f"Refusing hard-linked file (st_nlink={st.st_nlink}): {target}")
    ensure_within_resolved(repo_root.resolve(), target)
    return target


def validate_batch_name(batch: str) -> str:
    """Validate batch directory name as a single safe path segment."""
    if not BATCH_RE.fullmatch(batch) or ".." in batch:
        raise HelperError(
            "Invalid --batch name. Use a single folder name like 20261127_slice_name "
            "matching ^[A-Za-z0-9][A-Za-z0-9_.-]{0,127}$ without '..'."
        )
    return batch


def run_git(repo_root: Path, *args: str, check: bool = True) -> subprocess.CompletedProcess[str]:
    """Run a read-only git command without updating optional index stat locks."""
    proc = subprocess.run(
        ["git", "--no-optional-locks", *args],
        cwd=str(repo_root),
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    if check and proc.returncode != 0:
        msg = proc.stderr.strip() or proc.stdout.strip() or f"exit {proc.returncode}"
        raise HelperError(f"git {' '.join(args)} failed: {msg}")
    return proc


def resolve_commit(repo_root: Path, ref: str) -> tuple[str, str]:
    """Resolve commit ref to (full_sha, short8_sha)."""
    full = run_git(repo_root, "rev-parse", "--verify", f"{ref}^{{commit}}").stdout.strip()
    return full, full[:8]


def git_ancestor(repo_root: Path, ancestor_sha: str, descendant_sha: str) -> bool:
    """Return True if ancestor_sha is an ancestor of descendant_sha."""
    proc = run_git(
        repo_root,
        "merge-base",
        "--is-ancestor",
        ancestor_sha,
        descendant_sha,
        check=False,
    )
    return proc.returncode == 0


def current_branch(repo_root: Path) -> str:
    """Return current branch name or DETACHED."""
    proc = run_git(repo_root, "symbolic-ref", "--short", "HEAD", check=False)
    if proc.returncode == 0 and proc.stdout.strip():
        return proc.stdout.strip()
    return "DETACHED"


def upstream_info(repo_root: Path) -> tuple[str | None, str | None]:
    """Return (upstream_name, upstream_full_sha) if configured, else (None, None)."""
    proc = run_git(
        repo_root, "rev-parse", "--abbrev-ref", "--symbolic-full-name", "@{upstream}", check=False
    )
    if proc.returncode != 0 or not proc.stdout.strip():
        return None, None
    name = proc.stdout.strip()
    sha = run_git(repo_root, "rev-parse", "--verify", f"{name}^{{commit}}").stdout.strip()
    return name, sha


def touches_handoff_task_files(changed_files: list[str]) -> bool:
    """Return True if any changed file is under handoff/<task_dir>/... (excluding top README)."""
    for rel in changed_files:
        norm = rel.replace("\\", "/").lstrip("./")
        parts = norm.split("/")
        if len(parts) >= 3 and parts[0] == "handoff":
            return True
    return False


def read_text_lf_strict(path: Path) -> tuple[str | None, list[str]]:
    """Verify UTF-8 without BOM and pure LF line endings. Returns (text, errors)."""
    try:
        raw = path.read_bytes()
    except OSError as exc:
        return None, [f"{path}: cannot read file ({exc})."]
    errs: list[str] = []
    if raw.startswith(b"\xef\xbb\xbf"):
        errs.append(f"{path}: UTF-8 BOM is forbidden; expected UTF-8 without BOM.")
    if b"\r" in raw:
        errs.append(f"{path}: CR (CRLF) detected; expected pure LF line endings.")
    try:
        text = raw.decode("utf-8")
    except UnicodeDecodeError as exc:
        errs.append(f"{path}: invalid UTF-8 ({exc}).")
        return None, errs
    return text, errs


def normalize_lf_bytes(content: str) -> bytes:
    """Normalize line endings to LF without BOM and encode as UTF-8 bytes."""
    normalized = content.replace("\r\n", "\n").replace("\r", "\n")
    if not normalized.endswith("\n"):
        normalized += "\n"
    return normalized.encode("utf-8")


def write_exclusive_lf(path: Path, content: str) -> None:
    """Exclusively create path with O_CREAT | O_EXCL, refusing any pre-existing file/link."""
    data = normalize_lf_bytes(content)
    flags = os.O_WRONLY | os.O_CREAT | os.O_EXCL | getattr(os, "O_BINARY", 0)
    try:
        fd = os.open(str(path), flags, 0o644)
    except FileExistsError as exc:
        raise HelperError(f"{path} already exists; refusing exclusive creation.") from exc
    except OSError as exc:
        raise HelperError(f"Failed creating {path}: {exc}") from exc
    try:
        view = memoryview(data)
        written = 0
        while written < len(data):
            written += os.write(fd, view[written:])
        os.fsync(fd)
    finally:
        os.close(fd)


def write_atomic_lf(path: Path, content: str) -> None:
    """Atomically write UTF-8 LF text via same-directory O_EXCL temp file + os.replace."""
    data = normalize_lf_bytes(content)
    tmp_path = path.parent / f".{path.name}.{os.getpid()}.{uuid.uuid4().hex[:8]}.tmp"
    flags = os.O_WRONLY | os.O_CREAT | os.O_EXCL | getattr(os, "O_BINARY", 0)
    fd = os.open(str(tmp_path), flags, 0o644)
    replaced = False
    try:
        try:
            view = memoryview(data)
            written = 0
            while written < len(data):
                written += os.write(fd, view[written:])
            os.fsync(fd)
        finally:
            os.close(fd)
        os.replace(str(tmp_path), str(path))
        replaced = True
    finally:
        if not replaced:
            try:
                tmp_path.unlink(missing_ok=True)
            except OSError:
                pass


def iter_markdown_links(text: str) -> list[tuple[int, str]]:
    """Extract (1-based line_no, raw_target) outside fenced and inline code spans."""
    results: list[tuple[int, str]] = []
    fence_char: str | None = None
    fence_len = 0
    for idx, line in enumerate(text.splitlines(), start=1):
        fence_match = FENCE_RE.match(line)
        if fence_match:
            marker = fence_match.group(1)
            if fence_char is None:
                fence_char = marker[0]
                fence_len = len(marker)
            elif marker[0] == fence_char and len(marker) >= fence_len:
                fence_char = None
                fence_len = 0
            continue
        if fence_char is not None:
            continue
        cleaned = INLINE_CODE_RE.sub("", line)
        for match in MD_LINK_RE.finditer(cleaned):
            raw_inner = match.group(1).strip()
            if not raw_inner:
                results.append((idx, ""))
                continue
            if raw_inner.startswith("<"):
                if ">" not in raw_inner:
                    results.append((idx, ""))
                    continue
                target = raw_inner[1 : raw_inner.index(">")].strip()
            else:
                tokens = raw_inner.split()
                target = tokens[0] if tokens else ""
            results.append((idx, target))
    return results


def _reject_lexical_non_repo_target(target: str) -> str | None:
    """Return error reason before touching filesystem if target is non-relative."""
    if not target:
        return "empty link target"
    if "\x00" in target:
        return f"NUL byte in link target '{target}'"
    if WEB_SCHEME_RE.match(target):
        return None
    if ANY_SCHEME_RE.match(target):
        return f"non-relative URI or drive path '{target}' is forbidden"
    rel_part, _, _ = target.partition("#")
    if not rel_part:
        return None
    if rel_part.startswith(("/", "\\")):
        return f"rooted or UNC link path '{target}' is forbidden"
    win_path = PureWindowsPath(rel_part)
    if win_path.drive or win_path.is_absolute():
        return f"absolute Windows link path '{target}' is forbidden"
    return None


def check_markdown_links(repo_root: Path, md_path: Path, text: str) -> list[str]:
    """Validate relative workspace links without escaping repo_root or following reparse points."""
    errs: list[str] = []
    repo_real = repo_root.resolve()
    try:
        md_dir_rel = md_path.parent.resolve().relative_to(repo_real).as_posix()
    except ValueError:
        return [f"{md_path}: file lies outside repository root {repo_real}."]
    if md_dir_rel == ".":
        md_dir_rel = ""

    line_count_cache: dict[Path, int] = {}
    for line_no, target in iter_markdown_links(text):
        if WEB_SCHEME_RE.match(target):
            continue
        lexical_err = _reject_lexical_non_repo_target(target)
        if lexical_err:
            errs.append(f"{md_path}:{line_no}: {lexical_err}.")
            continue

        rel_part, has_hash, fragment = target.partition("#")
        if not rel_part:
            target_file = md_path
        else:
            joined = f"{md_dir_rel}/{rel_part}" if md_dir_rel else rel_part
            joined_posix = joined.replace("\\", "/")
            norm_posix = posixpath.normpath(joined_posix)
            if norm_posix == ".." or norm_posix.startswith("../") or posixpath.isabs(norm_posix):
                errs.append(
                    f"{md_path}:{line_no}: link target '{target}' lexically escapes repository root."
                )
                continue

            parts = [] if norm_posix == "." else [p for p in norm_posix.split("/") if p and p != "."]
            cur = repo_root
            traversal_failed = False
            for idx_part, part in enumerate(parts):
                cur = cur / part
                try:
                    st = os.lstat(cur)
                except FileNotFoundError:
                    errs.append(
                        f"{md_path}:{line_no}: broken link target '{target}' (missing {cur})."
                    )
                    traversal_failed = True
                    break
                except OSError as exc:
                    errs.append(
                        f"{md_path}:{line_no}: cannot stat link target '{target}' ({exc})."
                    )
                    traversal_failed = True
                    break
                if is_reparse_or_symlink(st):
                    errs.append(
                        f"{md_path}:{line_no}: link target '{target}' traverses symlink/reparse '{cur}'."
                    )
                    traversal_failed = True
                    break
                if idx_part < len(parts) - 1 and not stat.S_ISDIR(st.st_mode):
                    errs.append(
                        f"{md_path}:{line_no}: broken link target '{target}' (non-directory '{cur}')."
                    )
                    traversal_failed = True
                    break
            if traversal_failed:
                continue

            target_real = cur.resolve()
            try:
                target_real.relative_to(repo_real)
            except ValueError:
                errs.append(
                    f"{md_path}:{line_no}: resolved link target '{target}' escapes repository root."
                )
                continue
            target_file = target_real

        if has_hash and target_file.is_file():
            frag_match = LINE_FRAG_RE.fullmatch(fragment)
            if frag_match:
                start_line = int(frag_match.group(1))
                end_line = int(frag_match.group(2) or start_line)
                if start_line < 1 or end_line < start_line:
                    errs.append(
                        f"{md_path}:{line_no}: invalid line range '#{fragment}' in '{target}'."
                    )
                    continue
                if target_file not in line_count_cache:
                    try:
                        total_lines = len(
                            target_file.read_text(encoding="utf-8", errors="replace").splitlines()
                        )
                    except OSError as exc:
                        errs.append(
                            f"{md_path}:{line_no}: cannot read link target {target_file}: {exc}"
                        )
                        continue
                    line_count_cache[target_file] = total_lines
                total = line_count_cache[target_file]
                if end_line > total:
                    errs.append(
                        f"{md_path}:{line_no}: line anchor '#{fragment}' exceeds "
                        f"{target_file.name} ({total} lines)."
                    )
    return errs


def classify_commit_tag(subject: str) -> str:
    """Derive a concise tag from a commit subject."""
    if ":" in subject:
        return subject.split(":", 1)[0].strip()
    return "commit"


def build_submission_markdown(
    *,
    batch: str,
    branch: str,
    upstream_name: str | None,
    upstream_short: str | None,
    deferred_note: str,
    base_short: str,
    code_short: str,
    doc_short: str,
    post_action: str,
    profile: str,
    title: str,
    commits: list[tuple[str, str]],
    diff_stat: str,
    diff_check_ok: bool,
    changed_files: list[str],
    today: str,
) -> str:
    """Render 01_gemini_submission.md using strict relative link levels."""
    remote_display = (
        f"`{upstream_name}` @ `{upstream_short}`" if upstream_name and upstream_short else "无远端跟踪分支"
    )
    commit_rows = "\n".join(
        f"| {i} | `{sha}` | `{classify_commit_tag(subj)}` | `{subj}` |"
        for i, (sha, subj) in enumerate(commits, start=1)
    )
    if profile == "handoff-plan":
        checklist_note = (
            "- 本批 `review_profile=handoff-plan`，额外核对 "
            "[mcp-handoff-checklist.md](../../../.agents/skills/qkeymapper-codex-review/references/mcp-handoff-checklist.md)。\n"
        )
    elif any(f.replace("\\", "/") == "handoff/README.md" for f in changed_files):
        checklist_note = (
            "- 本批 `review_profile=general`，未涉及 `handoff/<任务目录>/` 下的主计划、能力矩阵或验证记录"
            "（仅包含顶层导航 `handoff/README.md` 入口引用）。\n"
        )
    else:
        checklist_note = (
            "- 本批 `review_profile=general`，不涉及 `handoff/<任务目录>/` 下的主计划、能力矩阵或验证记录。\n"
        )
    return f"""# Gemini 本地提交送审单：`{batch}`（`{title}`）

| 字段 | 值 |
|---|---|
| `generated_at` | `{today}` |
| `branch` | `{branch}` |
| `remote_tracking` | {remote_display} |
| `deferred_commits` | {deferred_note} |
| `review_range` | `{base_short}..{doc_short}`（共 {len(commits)} 条本地未推送提交） |
| `code_head` | `{code_short}` |
| `doc_head` | `{doc_short}` |
| `post_review_action` | `{post_action}` |
| `review_profile` | `{profile}` |

- **现场核对**：`git status --short` 为空；`git diff --check {base_short}..{doc_short}` {"通过（退出 0）" if diff_check_ok else "存在告警（见下方）"}；本批未执行 `git push`。
- **评审约束**：请 Codex 按 [qkeymapper-codex-review SKILL](../../../.agents/skills/qkeymapper-codex-review/SKILL.md) 执行只读评审，将结论写入 `02_codex_review_r1.md`（禁止修改跟踪文件或调用 UAC/gsudo）并更新 [LATEST.md](../LATEST.md)；收口或发回时严格继承 `post_review_action={post_action}`，并将简短「回传给 Gemini 的确认提示词」同时附在评审文件第 6 节及对用户的聊天回复末尾。

---

## 1. 待审本地提交清单

| 顺序 | SHA (8位) | 范围／类型 | 提交信息 |
|---|---|---|---|
{commit_rows}

### 差异统计（`git diff --stat {base_short}..{doc_short}`）

```text
{diff_stat.rstrip()}
```

## 2. 变更摘要与重点审阅项

{checklist_note}- TODO(Gemini): 补充核心改动说明、边界取舍与重点核验文件路径。

## 3. 验证与原始证据索引

- TODO(Gemini): 补充实际运行的单测/构建/产品验证结果、日志路径（`../../...`）及产物 SHA256 前 8 位。

## 4. 明确未验范围

- TODO(Gemini): 如实列明未执行的验证平台、驱动或功能边界。
"""


def build_latest_markdown(
    *,
    batch: str,
    title: str,
    branch: str,
    upstream_name: str | None,
    upstream_short: str | None,
    deferred_note: str,
    base_short: str,
    code_short: str,
    doc_short: str,
    post_action: str,
    profile: str,
    previous_closed: str,
) -> str:
    """Render out/codex_review/LATEST.md pointer document."""
    remote_line = (
        f"`{upstream_name}` @ `{upstream_short}`" if upstream_name and upstream_short else "无远端跟踪分支"
    )
    prev_line = f"\n- **前序已闭环批次**：{previous_closed}" if previous_closed else ""
    return f"""# Codex 本地提交 Review 交互入口

- **当前活跃评审批次**：`{batch}`（`{title}`）
- **当前分支与远端基线**：`{branch}`；远端 {remote_line}（未纳入本批的提前提交：{deferred_note}）
- **本轮冻结审查范围**：`{base_short}..{doc_short}`（代码终点 `{code_short}`，文档终点 `{doc_short}`）
- **终止边界（`post_review_action`）**：`{post_action}`
- **评审模式（`review_profile`）**：`{profile}`
- **当前评审状态**：**WAITING_R1_REVIEW**（等待 Codex 第 1 轮只读评审）
- **最新送审文档**：[{batch}/01_gemini_submission.md]({batch}/01_gemini_submission.md)
- **最新 Codex 评审／复审**：暂无（待写入 `{batch}/02_codex_review_r1.md`）{prev_line}
- **仓库权威协议**：[qkeymapper-codex-review SKILL](../../.agents/skills/qkeymapper-codex-review/SKILL.md)
- **当前约束（必读）**：
  1. Reviewer 默认严格只读：仅核对真实 `git log`/`git diff`、源码与已有 `out/` 产物，严禁修改任何 Git 跟踪文件、严禁 `git commit` / `git push`、严禁触发 UAC/gsudo。
  2. 结论写入 `{batch}/02_codex_review_r1.md` 并更新本页状态，严禁覆盖 `01_gemini_submission.md` 或任何历史批次原件；将简短「回传给 Gemini 的确认提示词」同时附在评审文件第 6 节及对用户的聊天回复末尾。
  3. 严格遵守 `post_review_action={post_action}`：评审结束或发回修复时直接按约定停止／交回，不得擅自追加后续无关切片指令。
"""


def extract_previous_closed(latest_path: Path) -> str:
    """Preserve prior closed batch summary from an already-validated LATEST.md."""
    if not os.path.lexists(latest_path):
        return ""
    text = latest_path.read_text(encoding="utf-8", errors="replace")
    for line in text.splitlines():
        if line.startswith("- **前序已闭环批次**："):
            return line.split("：", 1)[1].strip()
    return ""


def cmd_init(args: argparse.Namespace) -> int:
    repo_root = find_repo_root(args.repo_root)
    batch = validate_batch_name(args.batch)

    status_out = run_git(repo_root, "status", "--porcelain").stdout.strip()
    if status_out and not args.allow_dirty:
        raise HelperError(
            "Working tree or index is not clean (pass --allow-dirty only if explicitly intended):\n"
            + status_out
        )

    cur_head_full, cur_head_short = resolve_commit(repo_root, "HEAD")
    base_full, base_short = resolve_commit(repo_root, args.base)
    head_full, doc_short = resolve_commit(repo_root, args.head)
    if base_full == head_full:
        raise HelperError(f"Empty commit range: base and head resolve to the same commit {base_short}.")
    if not git_ancestor(repo_root, base_full, head_full):
        raise HelperError(f"Base commit {base_short} is not an ancestor of {doc_short}.")
    if not git_ancestor(repo_root, head_full, cur_head_full):
        raise HelperError(
            f"Head commit {doc_short} is not an ancestor of current HEAD {cur_head_short}."
        )

    code_ref = args.code_head or args.head
    code_full, code_short = resolve_commit(repo_root, code_ref)
    if not git_ancestor(repo_root, base_full, code_full) or not git_ancestor(repo_root, code_full, head_full):
        raise HelperError("--code-head must satisfy base <= code_head <= head.")

    up_name, up_full = upstream_info(repo_root)
    up_short = up_full[:8] if up_full else None
    if up_full:
        if git_ancestor(repo_root, head_full, up_full):
            raise HelperError(
                f"Target range endpoint {doc_short} is already pushed to {up_name}@{up_short}."
            )
        if not git_ancestor(repo_root, up_full, base_full):
            raise HelperError(
                f"Upstream {up_name}@{up_short} is not an ancestor of base {base_short}; "
                "refusing diverged or stale unpushed range."
            )
        if up_full == base_full:
            deferred_note = "无（起点即当前基线）"
        else:
            deferred_lines = run_git(
                repo_root, "log", "--oneline", f"{up_full}..{base_full}"
            ).stdout.splitlines()
            deferred_note = (
                f"`{up_short}..{base_short}`（共 {len(deferred_lines)} 条先导本地提交延后至后续批次回顾，不纳入本批）"
            )
    else:
        deferred_note = "无远端跟踪分支（请人工核对本地起点基线）"

    changed_files = [
        line.strip()
        for line in run_git(
            repo_root, "diff", "--name-only", f"{base_full}..{head_full}"
        ).stdout.splitlines()
        if line.strip()
    ]
    if touches_handoff_task_files(changed_files) and args.profile != "handoff-plan":
        raise HelperError(
            f"Range {base_short}..{doc_short} modifies task files under handoff/<task>/; "
            "re-run with --profile handoff-plan."
        )

    log_proc = run_git(repo_root, "log", "--reverse", "--format=%H%x09%s", f"{base_full}..{head_full}")
    commits: list[tuple[str, str]] = []
    for line in log_proc.stdout.splitlines():
        if not line.strip():
            continue
        sha, _, subj = line.partition("\t")
        commits.append((sha[:8], subj))
    if not commits:
        raise HelperError(f"No commits found in range {base_short}..{doc_short}.")

    diff_stat = run_git(repo_root, "diff", "--stat", f"{base_full}..{head_full}").stdout
    diff_check_proc = run_git(repo_root, "diff", "--check", f"{base_full}..{head_full}", check=False)
    diff_check_ok = diff_check_proc.returncode == 0
    branch = current_branch(repo_root)

    # Validate ALL directory chains and existing files BEFORE any write occurs.
    review_root = validate_safe_dir_chain(repo_root, ("out", "codex_review"), create_missing=True)
    latest_path = review_root / "LATEST.md"
    previous_closed = ""
    if not args.no_update_latest and os.path.lexists(latest_path):
        validated_latest = validate_safe_existing_file(
            repo_root, ("out", "codex_review", "LATEST.md"), reject_hardlinks=True
        )
        previous_closed = extract_previous_closed(validated_latest)

    batch_dir = validate_safe_dir_chain(repo_root, ("out", "codex_review", batch), create_missing=True)
    submission_path = batch_dir / "01_gemini_submission.md"
    if os.path.lexists(submission_path):
        raise HelperError(
            f"{submission_path} already exists; refusing to overwrite existing submission."
        )

    title = args.title or batch
    today = datetime.now().strftime("%Y-%m-%d")
    submission_text = build_submission_markdown(
        batch=batch,
        branch=branch,
        upstream_name=up_name,
        upstream_short=up_short,
        deferred_note=deferred_note,
        base_short=base_short,
        code_short=code_short,
        doc_short=doc_short,
        post_action=args.post_action,
        profile=args.profile,
        title=title,
        commits=commits,
        diff_stat=diff_stat,
        diff_check_ok=diff_check_ok,
        changed_files=changed_files,
        today=today,
    )
    latest_text = (
        build_latest_markdown(
            batch=batch,
            title=title,
            branch=branch,
            upstream_name=up_name,
            upstream_short=up_short,
            deferred_note=deferred_note,
            base_short=base_short,
            code_short=code_short,
            doc_short=doc_short,
            post_action=args.post_action,
            profile=args.profile,
            previous_closed=previous_closed,
        )
        if not args.no_update_latest
        else ""
    )

    write_exclusive_lf(submission_path, submission_text)
    if not args.no_update_latest:
        write_atomic_lf(latest_path, latest_text)

    print(
        json.dumps(
            {
                "ok": True,
                "batch": batch,
                "submission": str(submission_path),
                "latest_updated": not args.no_update_latest,
                "commits": len(commits),
                "range": f"{base_short}..{doc_short}",
                "post_review_action": args.post_action,
                "review_profile": args.profile,
            },
            ensure_ascii=False,
        )
    )
    return 0


def expected_round_filename(seq: int) -> str:
    """Return canonical filename for 1-based sequence index."""
    if seq == 1:
        return "01_gemini_submission.md"
    if seq == 2:
        return "02_codex_review_r1.md"
    rnd = (seq - 1) // 2
    if seq % 2 == 1:
        return f"{seq:02d}_gemini_response_r{rnd}.md"
    return f"{seq:02d}_codex_recheck_r{rnd}.md"


def check_round_sequence(files: list[Path], batch_dir: Path) -> list[str]:
    """Verify strict 1-based canonical filenames and round numbering."""
    errs: list[str] = []
    parsed_items: list[tuple[int, Path]] = []
    for path in sorted(files, key=lambda p: p.name):
        match = ROUND_FILE_RE.fullmatch(path.name)
        if not match:
            errs.append(f"{path}: filename does not follow canonical round naming convention.")
            continue
        num = int(match.group(1))
        if num < 1:
            errs.append(f"{path}: sequence index must be >= 01.")
            continue
        parsed_items.append((num, path))

    parsed_items.sort(key=lambda item: (item[0], item[1].name))
    seen: set[int] = set()
    for expected_idx, (num, path) in enumerate(parsed_items, start=1):
        if num in seen:
            errs.append(f"{path}: duplicate sequence index {num:02d} in {batch_dir.name}.")
            continue
        seen.add(num)
        if num != expected_idx:
            errs.append(f"{path}: expected sequence {expected_idx:02d} without gaps, found {num:02d}.")
        expected_name = expected_round_filename(num)
        if path.name != expected_name:
            errs.append(
                f"{path}: non-canonical filename for slot {num:02d}; expected '{expected_name}'."
            )

    if not any(p.name == "01_gemini_submission.md" for p in files):
        errs.append(f"{batch_dir}: missing canonical '01_gemini_submission.md'.")
    return errs


def parse_submission_table(text: str) -> dict[str, str]:
    """Extract backtick-keyed metadata rows from 01_gemini_submission.md."""
    meta: dict[str, str] = {}
    for line in text.splitlines():
        match = TABLE_KV_RE.match(line)
        if match:
            meta[match.group(1)] = match.group(2).strip()
    return meta


def strip_backticks(value: str) -> str:
    """Strip single surrounding backticks from scalar markdown table cells."""
    v = value.strip()
    if len(v) >= 2 and v.startswith("`") and v.endswith("`"):
        return v[1:-1].strip()
    return v


def check_submission_metadata(
    repo_root: Path, submission_path: Path, text: str
) -> tuple[list[str], dict[str, str]]:
    """Strictly verify frozen SHAs, branch membership, upstream, profile, and post_review_action."""
    errs: list[str] = []
    meta = parse_submission_table(text)
    for key in REQUIRED_SUBMISSION_KEYS:
        if not meta.get(key):
            errs.append(f"{submission_path}: missing required metadata row '{key}'.")
    if errs:
        return errs, meta

    post_action = strip_backticks(meta["post_review_action"])
    if post_action not in POST_ACTIONS:
        errs.append(
            f"{submission_path}: invalid post_review_action '{post_action}'; expected one of {POST_ACTIONS}."
        )

    profile = strip_backticks(meta["review_profile"])
    if profile not in PROFILES:
        errs.append(
            f"{submission_path}: invalid review_profile '{profile}'; expected one of {PROFILES}."
        )

    range_match = HEX_RANGE_RE.match(meta["review_range"])
    if not range_match:
        errs.append(
            f"{submission_path}: review_range must start with fixed `<hex>..<hex>` SHAs "
            f"(got {meta['review_range']!r}); drifting refs like HEAD are forbidden."
        )
        return errs, meta

    base_raw, doc_range_raw = range_match.group(1), range_match.group(2)
    code_raw = strip_backticks(meta["code_head"])
    doc_raw = strip_backticks(meta["doc_head"])
    if not HEX_SHA_RE.fullmatch(code_raw):
        errs.append(f"{submission_path}: code_head must be a hex SHA (got {meta['code_head']!r}).")
        return errs, meta
    if not HEX_SHA_RE.fullmatch(doc_raw):
        errs.append(f"{submission_path}: doc_head must be a hex SHA (got {meta['doc_head']!r}).")
        return errs, meta

    try:
        base_full, base_short = resolve_commit(repo_root, base_raw)
        range_end_full, range_end_short = resolve_commit(repo_root, doc_range_raw)
        code_full, code_short = resolve_commit(repo_root, code_raw)
        doc_full, doc_short = resolve_commit(repo_root, doc_raw)
        cur_head_full, cur_head_short = resolve_commit(repo_root, "HEAD")
    except HelperError as exc:
        errs.append(f"{submission_path}: {exc}")
        return errs, meta

    if doc_full != range_end_full:
        errs.append(
            f"{submission_path}: doc_head ({doc_short}) does not match review_range endpoint ({range_end_short})."
        )
    if base_full == doc_full:
        errs.append(f"{submission_path}: review_range {base_short}..{doc_short} is empty.")
    if not git_ancestor(repo_root, base_full, code_full) or not git_ancestor(repo_root, code_full, doc_full):
        errs.append(
            f"{submission_path}: commits must satisfy base ({base_short}) <= code_head ({code_short}) <= doc_head ({doc_short})."
        )
    if not git_ancestor(repo_root, doc_full, cur_head_full):
        errs.append(
            f"{submission_path}: doc_head ({doc_short}) does not belong to current HEAD ({cur_head_short}) history."
        )

    branch_val = strip_backticks(meta["branch"])
    actual_branch = current_branch(repo_root)
    if actual_branch != "DETACHED" and branch_val != actual_branch:
        errs.append(
            f"{submission_path}: branch '{branch_val}' does not match active branch '{actual_branch}'."
        )

    up_name, up_full = upstream_info(repo_root)
    remote_raw = meta["remote_tracking"].strip()
    deferred_raw = meta["deferred_commits"].strip()
    if up_full:
        up_short = up_full[:8]
        remote_match = REMOTE_META_RE.fullmatch(remote_raw)
        if not remote_match:
            errs.append(
                f"{submission_path}: remote_tracking must record `<ref>` @ `<hex>` (got {remote_raw!r})."
            )
        else:
            recorded_ref, recorded_sha_raw = remote_match.group(1), remote_match.group(2)
            try:
                recorded_full, recorded_short = resolve_commit(repo_root, recorded_sha_raw)
                if up_name and recorded_ref != up_name:
                    errs.append(
                        f"{submission_path}: remote_tracking ref '{recorded_ref}' differs from upstream '{up_name}'."
                    )
                if recorded_full != up_full and not git_ancestor(repo_root, recorded_full, base_full):
                    errs.append(
                        f"{submission_path}: remote_tracking SHA {recorded_short} is invalid relative to base {base_short}."
                    )
            except HelperError as exc:
                errs.append(f"{submission_path}: {exc}")
        if git_ancestor(repo_root, doc_full, up_full):
            errs.append(
                f"{submission_path}: doc_head ({doc_short}) is already reachable from upstream {up_name}@{up_short}."
            )
        if not git_ancestor(repo_root, up_full, base_full):
            errs.append(
                f"{submission_path}: upstream {up_name}@{up_short} is not an ancestor of base ({base_short})."
            )
        elif up_full != base_full and f"{up_short}..{base_short}" not in deferred_raw:
            errs.append(
                f"{submission_path}: deferred_commits must explicitly record `{up_short}..{base_short}` "
                "when local commits precede base."
            )
    else:
        if "无远端跟踪分支" not in remote_raw:
            errs.append(
                f"{submission_path}: repository has no upstream branch; remote_tracking must explicitly state 无远端跟踪分支."
            )

    parsed_info = {
        "base_full": base_full,
        "base_short": base_short,
        "doc_full": doc_full,
        "post_review_action": post_action,
        "review_profile": profile,
    }
    return errs, parsed_info


def check_latest_pointer(
    repo_root: Path,
    latest_path: Path,
    latest_text: str,
    batch: str,
    submission_info: dict[str, str],
) -> list[str]:
    """Cross-check structured active fields in LATEST.md against the target batch."""
    errs: list[str] = []
    fields: dict[str, str] = {}
    for line in latest_text.splitlines():
        if line.startswith("- **") and "**：" in line:
            label_part, _, val_part = line.partition("**：")
            label = label_part[4:].strip()
            fields[label] = val_part.strip()

    active_val = fields.get("当前活跃评审批次", "")
    if not active_val.startswith(f"`{batch}`"):
        errs.append(
            f"{latest_path}: active batch field must start with `{batch}` (found {active_val!r})."
        )

    post_val = strip_backticks(fields.get("终止边界（`post_review_action`）", ""))
    expected_post = submission_info.get("post_review_action", "")
    if expected_post and post_val != expected_post:
        errs.append(
            f"{latest_path}: post_review_action '{post_val}' does not match submission '{expected_post}'."
        )

    range_val = fields.get("本轮冻结审查范围", "")
    range_match = re.match(
        r"^`([0-9a-fA-F]{7,40})\.\.([0-9a-fA-F]{7,40})`.*代码终点\s*`([0-9a-fA-F]{7,40})`.*文档终点\s*`([0-9a-fA-F]{7,40})`",
        range_val,
    )
    if not range_match:
        errs.append(
            f"{latest_path}: 本轮冻结审查范围 must record fixed `<base>..<doc>` with code & doc hex SHAs."
        )
    else:
        l_base_raw, l_end_raw, l_code_raw, l_doc_raw = range_match.groups()
        try:
            l_base_full, l_base_short = resolve_commit(repo_root, l_base_raw)
            l_end_full, l_end_short = resolve_commit(repo_root, l_end_raw)
            l_code_full, l_code_short = resolve_commit(repo_root, l_code_raw)
            l_doc_full, l_doc_short = resolve_commit(repo_root, l_doc_raw)
            cur_head_full, cur_head_short = resolve_commit(repo_root, "HEAD")
        except HelperError as exc:
            errs.append(f"{latest_path}: {exc}")
        else:
            if submission_info.get("base_full") and l_base_full != submission_info["base_full"]:
                errs.append(
                    f"{latest_path}: base {l_base_short} differs from submission base {submission_info['base_short']}."
                )
            if l_end_full != l_doc_full:
                errs.append(
                    f"{latest_path}: range endpoint {l_end_short} differs from doc_head {l_doc_short}."
                )
            if not git_ancestor(repo_root, l_base_full, l_code_full) or not git_ancestor(repo_root, l_code_full, l_doc_full):
                errs.append(
                    f"{latest_path}: commits must satisfy base <= code_head <= doc_head."
                )
            if submission_info.get("doc_full") and not git_ancestor(repo_root, submission_info["doc_full"], l_doc_full):
                errs.append(
                    f"{latest_path}: doc_head ({l_doc_short}) precedes initial submission endpoint."
                )
            if not git_ancestor(repo_root, l_doc_full, cur_head_full):
                errs.append(
                    f"{latest_path}: doc_head ({l_doc_short}) does not belong to current HEAD ({cur_head_short})."
                )

    sub_doc_field = fields.get("最新送审／回复文档", "") or fields.get("最新送审文档", "")
    if f"({batch}/" not in sub_doc_field:
        errs.append(f"{latest_path}: latest author link must target '{batch}/...'.")
    return errs


def _repo_rel_parts_from_arg(repo_root: Path, raw_file: str) -> tuple[str, ...]:
    """Convert raw --files CLI argument to safe relative path parts inside repo_root."""
    if "\x00" in raw_file:
        raise HelperError(f"Invalid path with NUL byte: {raw_file!r}")
    p = Path(raw_file)
    if p.is_absolute():
        try:
            rel = p.relative_to(repo_root)
        except ValueError as exc:
            raise HelperError(f"Refusing path outside repository root: {raw_file}") from exc
    else:
        rel = p
    if rel.drive or rel.is_absolute():
        raise HelperError(f"Refusing non-relative path: {raw_file}")
    parts = [part for part in rel.parts if part not in ("", ".")]
    if not parts or any(part == ".." for part in parts):
        raise HelperError(f"Refusing path traversal outside repository root: {raw_file}")
    return tuple(parts)


def cmd_check(args: argparse.Namespace) -> int:
    repo_root = find_repo_root(args.repo_root)
    errors: list[str] = []
    warnings: list[str] = []
    checked_files: list[str] = []

    if args.files is not None:
        if not args.files:
            raise HelperError("--files requires at least one file path.")
        for raw_file in args.files:
            try:
                rel_parts = _repo_rel_parts_from_arg(repo_root, raw_file)
                safe_path = validate_safe_existing_file(repo_root, rel_parts)
            except HelperError as exc:
                errors.append(str(exc))
                continue
            text, fmt_errs = read_text_lf_strict(safe_path)
            errors.extend(fmt_errs)
            if text is not None:
                errors.extend(check_markdown_links(repo_root, safe_path, text))
            checked_files.append(str(safe_path))
    else:
        if not args.batch:
            raise HelperError("Specify either --batch <name> or --files <path...>.")
        batch = validate_batch_name(args.batch)
        batch_dir = validate_safe_dir_chain(repo_root, ("out", "codex_review", batch), create_missing=False)
        batch_real = batch_dir.resolve()

        md_files: list[Path] = []
        try:
            entries = sorted(batch_dir.iterdir(), key=lambda p: p.name)
        except OSError as exc:
            raise HelperError(f"Failed listing {batch_dir}: {exc}") from exc

        for entry in entries:
            if not entry.name.endswith(".md"):
                continue
            try:
                st = os.lstat(entry)
            except OSError as exc:
                errors.append(f"{entry}: cannot stat entry ({exc}).")
                continue
            if is_reparse_or_symlink(st):
                errors.append(f"{entry}: symlink or reparse markdown file is forbidden.")
                continue
            if not stat.S_ISREG(st.st_mode):
                errors.append(f"{entry}: non-regular markdown entry is forbidden.")
                continue
            if entry.resolve().parent != batch_real:
                errors.append(f"{entry}: resolved path leaves batch directory {batch_real}.")
                continue
            md_files.append(entry)

        if not md_files:
            raise HelperError(f"No valid regular markdown files found in {batch_dir}")

        errors.extend(check_round_sequence(md_files, batch_dir))
        submission_info: dict[str, str] = {}
        has_canonical_submission = False

        for path in md_files:
            text, fmt_errs = read_text_lf_strict(path)
            errors.extend(fmt_errs)
            if text is not None:
                errors.extend(check_markdown_links(repo_root, path, text))
                if path.name == "01_gemini_submission.md":
                    has_canonical_submission = True
                    if args.strict_meta:
                        meta_errs, submission_info = check_submission_metadata(repo_root, path, text)
                        errors.extend(meta_errs)
            checked_files.append(str(path))

        if args.strict_meta and not has_canonical_submission:
            errors.append(f"{batch_dir}: --strict-meta requires canonical '01_gemini_submission.md'.")

        if args.check_latest:
            try:
                latest_path = validate_safe_existing_file(
                    repo_root, ("out", "codex_review", "LATEST.md")
                )
            except HelperError as exc:
                errors.append(str(exc))
            else:
                text, fmt_errs = read_text_lf_strict(latest_path)
                errors.extend(fmt_errs)
                if text is not None:
                    errors.extend(check_markdown_links(repo_root, latest_path, text))
                    errors.extend(
                        check_latest_pointer(repo_root, latest_path, text, batch, submission_info)
                    )
                checked_files.append(str(latest_path))

    result = {
        "ok": len(errors) == 0,
        "checked_count": len(checked_files),
        "error_count": len(errors),
        "warning_count": len(warnings),
        "errors": errors,
        "warnings": warnings,
    }
    if args.json:
        print(json.dumps(result, indent=2, ensure_ascii=False))
    else:
        print(
            f"CHECK {'PASS' if result['ok'] else 'FAIL'}: "
            f"{result['checked_count']} files, {result['error_count']} errors, {result['warning_count']} warnings"
        )
        for item in errors:
            print(f"ERROR: {item}")
        for item in warnings:
            print(f"WARN: {item}")
    return 0 if result["ok"] else 1


def cmd_status(args: argparse.Namespace) -> int:
    repo_root = find_repo_root(args.repo_root)
    out_dir = repo_root / "out"
    review_dir = out_dir / "codex_review"

    review_root: Path | None = None
    if os.path.lexists(review_dir):
        review_root = validate_safe_dir_chain(repo_root, ("out", "codex_review"), create_missing=False)

    _, head_short = resolve_commit(repo_root, "HEAD")
    branch = current_branch(repo_root)
    up_name, up_full = upstream_info(repo_root)
    dirty = bool(run_git(repo_root, "status", "--porcelain").stdout.strip())

    batches: list[dict[str, Any]] = []
    skipped_entries: list[str] = []
    if review_root is not None:
        for entry in sorted(review_root.iterdir(), key=lambda p: p.name):
            if entry.name.startswith("."):
                continue
            try:
                st = os.lstat(entry)
            except OSError:
                continue
            if is_reparse_or_symlink(st):
                skipped_entries.append(f"{entry.name} (reparse/symlink skipped)")
                continue
            if not stat.S_ISDIR(st.st_mode):
                continue
            files: list[str] = []
            for child in sorted(entry.iterdir(), key=lambda p: p.name):
                if not child.name.endswith(".md"):
                    continue
                cst = os.lstat(child)
                if not is_reparse_or_symlink(cst) and stat.S_ISREG(cst.st_mode):
                    files.append(child.name)
            batches.append({"batch": entry.name, "files": files})

    latest_excerpt = ""
    latest_exists = False
    if review_root is not None and os.path.lexists(review_root / "LATEST.md"):
        safe_latest = validate_safe_existing_file(repo_root, ("out", "codex_review", "LATEST.md"))
        latest_exists = True
        latest_text, fmt_errs = read_text_lf_strict(safe_latest)
        if latest_text is None:
            raise HelperError("; ".join(fmt_errs))
        latest_excerpt = latest_text.strip()

    payload = {
        "repo_root": str(repo_root),
        "branch": branch,
        "head": head_short,
        "upstream": f"{up_name}@{up_full[:8]}" if up_name and up_full else None,
        "dirty": dirty,
        "latest_exists": latest_exists,
        "batches": batches,
        "skipped_entries": skipped_entries,
    }
    if args.json:
        print(json.dumps(payload, indent=2, ensure_ascii=False))
    else:
        print(
            f"Repo={repo_root} | Branch={branch} | HEAD={head_short} | "
            f"Upstream={payload['upstream']} | Dirty={dirty}"
        )
        print(f"Batches under out/codex_review/: {len(batches)}")
        for b in batches:
            print(f"  - {b['batch']}: {', '.join(b['files'])}")
        for s in skipped_entries:
            print(f"WARN: skipped {s}")
        if latest_excerpt:
            print("--- LATEST.md ---")
            print(latest_excerpt)
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Non-elevated helper for out/codex_review/ Gemini <-> Codex commit review workflows."
    )
    sub = parser.add_subparsers(dest="command", required=True)

    p_init = sub.add_parser("init", help="Initialize out/codex_review/<batch>/01_gemini_submission.md & LATEST.md")
    p_init.add_argument("--batch", required=True, help="Single folder name under out/codex_review/")
    p_init.add_argument("--base", required=True, help="Frozen base commit SHA or ref (exclusive)")
    p_init.add_argument("--head", default="HEAD", help="Frozen head commit SHA or ref (inclusive)")
    p_init.add_argument("--code-head", default=None, help="Optional code head commit SHA between base and head")
    p_init.add_argument("--post-action", choices=POST_ACTIONS, default="pause_for_user")
    p_init.add_argument("--profile", choices=PROFILES, default="general")
    p_init.add_argument("--title", default=None, help="Human-readable batch title")
    p_init.add_argument("--allow-dirty", action="store_true", help="Allow dirty working tree during init")
    p_init.add_argument("--no-update-latest", action="store_true", help="Do not overwrite LATEST.md")
    p_init.add_argument("--repo-root", type=Path, default=None, help="Override repository root (for tests)")
    p_init.set_defaults(func=cmd_init)

    p_check = sub.add_parser("check", help="Validate review markdown encoding, LF, sequence, links, and line bounds")
    p_check.add_argument("--batch", default=None, help="Batch folder name under out/codex_review/")
    p_check.add_argument("--files", nargs="*", default=None, help="Explicit markdown files relative to repo root")
    p_check.add_argument("--check-latest", action="store_true", help="Also verify out/codex_review/LATEST.md")
    p_check.add_argument("--strict-meta", action="store_true", help="Strictly verify frozen SHAs, branch/upstream, and post_review_action")
    p_check.add_argument("--json", action="store_true", help="Emit JSON result")
    p_check.add_argument("--repo-root", type=Path, default=None, help="Override repository root (for tests)")
    p_check.set_defaults(func=cmd_check)

    p_status = sub.add_parser("status", help="Show active review status and Git unpushed state (read-only)")
    p_status.add_argument("--json", action="store_true", help="Emit JSON output")
    p_status.add_argument("--repo-root", type=Path, default=None, help="Override repository root (for tests)")
    p_status.set_defaults(func=cmd_status)

    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    try:
        return args.func(args)
    except HelperError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
