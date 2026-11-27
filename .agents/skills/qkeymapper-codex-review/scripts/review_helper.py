#!/usr/bin/env python3
"""Helper CLI for QKeyMapper local Gemini <-> Codex commit review workflows.

Strict non-elevated utility using Python standard library only. Never executes
git commit, git push, or elevated uac/gsudo commands. All generated and checked
review workspace artifacts live under <repo_root>/out/codex_review/.
"""

from __future__ import annotations

import argparse
from datetime import datetime
import json
from pathlib import Path
import re
import subprocess
import sys
from typing import Any


BATCH_RE = re.compile(r"^[A-Za-z0-9][A-Za-z0-9_.-]{0,127}$")
ROUND_FILE_RE = re.compile(
    r"^(\d{2})_(gemini|codex)_(submission|review|response|recheck)(?:_r(\d+))?\.md$"
)
FENCE_RE = re.compile(r"^\s*(`{3,}|~{3,})")
INLINE_CODE_RE = re.compile(r"`+[^`]*`+")
MD_LINK_RE = re.compile(r"\[[^\]]*\]\(([^)]+)\)")
LINE_FRAG_RE = re.compile(r"^L(\d+)(?:-L(\d+))?$")
SCHEME_RE = re.compile(r"^[A-Za-z][A-Za-z0-9+.-]*:")

POST_ACTIONS = ("pause_for_user", "fix_only_then_pause", "continue_after_pass")
PROFILES = ("general", "handoff-plan")


class HelperError(Exception):
    """Domain error with a concise user-facing message."""


def find_repo_root(start: Path | None = None) -> Path:
    """Resolve the repository root containing .git from CWD or explicit start."""
    cur = (start or Path.cwd()).resolve()
    for candidate in (cur, *cur.parents):
        if (candidate / ".git").exists():
            return candidate
    raise HelperError(f"Could not locate Git repository root from {cur}")


def ensure_within(base: Path, target: Path) -> Path:
    """Ensure target resolves strictly inside base directory."""
    base_res = base.resolve()
    target_res = target.resolve()
    try:
        target_res.relative_to(base_res)
    except ValueError as exc:
        raise HelperError(f"Refusing path outside {base_res}: {target_res}") from exc
    return target_res


def validate_batch_name(batch: str) -> str:
    """Validate batch directory name as a single safe path segment."""
    if not BATCH_RE.fullmatch(batch) or ".." in batch:
        raise HelperError(
            "Invalid --batch name. Use a single folder name like 20261127_slice_name "
            "matching ^[A-Za-z0-9][A-Za-z0-9_.-]{0,127}$ without '..'."
        )
    return batch


def run_git(repo_root: Path, *args: str, check: bool = True) -> subprocess.CompletedProcess[str]:
    """Run a read-only git command with UTF-8 decoding."""
    proc = subprocess.run(
        ["git", *args],
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


def read_text_lf_strict(path: Path) -> tuple[str | None, list[str]]:
    """Verify UTF-8 without BOM and pure LF line endings. Returns (text, errors)."""
    raw = path.read_bytes()
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


def write_text_lf(path: Path, content: str) -> None:
    """Write UTF-8 without BOM and pure LF line endings."""
    path.parent.mkdir(parents=True, exist_ok=True)
    normalized = content.replace("\r\n", "\n").replace("\r", "\n")
    if not normalized.endswith("\n"):
        normalized += "\n"
    with path.open("w", encoding="utf-8", newline="\n") as fh:
        fh.write(normalized)


def iter_markdown_links(text: str) -> list[tuple[int, str]]:
    """Extract (1-based line_no, raw_target) for links outside fenced & inline code."""
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
            raw_target = match.group(1).strip()
            if raw_target.startswith("<") and ">" in raw_target:
                raw_target = raw_target[1 : raw_target.index(">")].strip()
            else:
                # Strip optional markdown link title: path "title"
                raw_target = raw_target.split()[0]
            results.append((idx, raw_target))
    return results


def check_markdown_links(md_path: Path, text: str) -> list[str]:
    """Validate relative file links and optional #L<n>[-L<m>] line bounds."""
    errs: list[str] = []
    line_count_cache: dict[Path, int] = {}
    for line_no, target in iter_markdown_links(text):
        if not target or SCHEME_RE.match(target):
            continue
        rel_part, has_hash, fragment = target.partition("#")
        if not rel_part:
            target_file = md_path
        else:
            target_file = (md_path.parent / rel_part).resolve()
            if not target_file.exists():
                errs.append(f"{md_path}:{line_no}: broken link target '{target}' (resolved {target_file}).")
                continue
        if has_hash and target_file.is_file():
            frag_match = LINE_FRAG_RE.fullmatch(fragment)
            if frag_match:
                start_line = int(frag_match.group(1))
                end_line = int(frag_match.group(2) or start_line)
                if start_line < 1 or end_line < start_line:
                    errs.append(f"{md_path}:{line_no}: invalid line range '#{fragment}' in '{target}'.")
                    continue
                if target_file not in line_count_cache:
                    try:
                        total_lines = len(target_file.read_text(encoding="utf-8", errors="replace").splitlines())
                    except OSError as exc:
                        errs.append(f"{md_path}:{line_no}: cannot read link target {target_file}: {exc}")
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
    checklist_note = (
        "- 额外核对 [mcp-handoff-checklist.md](../../../.agents/skills/qkeymapper-codex-review/references/mcp-handoff-checklist.md)。\n"
        if profile == "handoff-plan"
        else "- 本批 `review_profile=general`，不涉及 `handoff/` 主计划或能力矩阵变更。\n"
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
    """Preserve prior closed batch summary from existing LATEST.md if present."""
    if not latest_path.is_file():
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

    base_full, base_short = resolve_commit(repo_root, args.base)
    head_full, doc_short = resolve_commit(repo_root, args.head)
    if base_full == head_full:
        raise HelperError(f"Empty commit range: base and head resolve to the same commit {base_short}.")
    if not git_ancestor(repo_root, base_full, head_full):
        raise HelperError(f"Base commit {base_short} is not an ancestor of {doc_short}.")

    code_ref = args.code_head or args.head
    code_full, code_short = resolve_commit(repo_root, code_ref)
    if not git_ancestor(repo_root, base_full, code_full) or not git_ancestor(repo_root, code_full, head_full):
        raise HelperError("--code-head must satisfy base <= code_head <= head.")

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
    up_name, up_full = upstream_info(repo_root)
    up_short = up_full[:8] if up_full else None
    if up_full and up_full != base_full:
        if git_ancestor(repo_root, up_full, base_full):
            deferred_lines = run_git(
                repo_root, "log", "--oneline", f"{up_full}..{base_full}"
            ).stdout.splitlines()
            deferred_note = (
                f"`{up_short}..{base_short}`（共 {len(deferred_lines)} 条先导本地提交延后至后续批次回顾，不纳入本批）"
            )
        else:
            deferred_note = f"远端 `{up_short}` 与基线 `{base_short}` 非直系前后继，请人工核对范围"
    else:
        deferred_note = "无（起点即当前基线）"

    review_root = ensure_within(repo_root, repo_root / "out" / "codex_review")
    batch_dir = ensure_within(review_root, review_root / batch)
    submission_path = ensure_within(batch_dir, batch_dir / "01_gemini_submission.md")
    if submission_path.exists() and not args.force:
        raise HelperError(
            f"{submission_path} already exists; refusing to overwrite (pass --force to replace)."
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
        today=today,
    )
    write_text_lf(submission_path, submission_text)

    latest_path = ensure_within(review_root, review_root / "LATEST.md")
    if not args.no_update_latest:
        previous_closed = extract_previous_closed(latest_path)
        latest_text = build_latest_markdown(
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
        write_text_lf(latest_path, latest_text)

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
            },
            ensure_ascii=False,
        )
    )
    return 0


def check_round_sequence(files: list[Path], batch_dir: Path) -> tuple[list[str], list[str]]:
    """Verify round file naming 01/02/03/... ordering cleanly."""
    errs: list[str] = []
    warns: list[str] = []
    numbered: list[tuple[int, Path, str, str]] = []
    for path in sorted(files, key=lambda p: p.name):
        match = ROUND_FILE_RE.fullmatch(path.name)
        if not match:
            warns.append(f"{path}: filename does not follow NN_<role>_<kind>[_rN].md convention.")
            continue
        num = int(match.group(1))
        role = match.group(2)
        kind = match.group(3)
        if num % 2 == 1 and role != "gemini":
            errs.append(f"{path}: odd sequence {num:02d} must belong to gemini (submission/response).")
        if num % 2 == 0 and role != "codex":
            errs.append(f"{path}: even sequence {num:02d} must belong to codex (review/recheck).")
        if num == 1 and kind != "submission":
            errs.append(f"{path}: file 01 must be submission.")
        numbered.append((num, path, role, kind))

    numbered.sort(key=lambda item: item[0])
    seen: set[int] = set()
    for idx, (num, path, _, _) in enumerate(numbered, start=1):
        if num in seen:
            errs.append(f"{path}: duplicate sequence index {num:02d} in {batch_dir.name}.")
        seen.add(num)
        if num != idx:
            errs.append(f"{path}: expected sequence {idx:02d} without gaps, found {num:02d}.")
    if numbered and not any(item[0] == 1 for item in numbered):
        errs.append(f"{batch_dir}: missing initial 01_gemini_submission.md.")
    return errs, warns


def check_submission_metadata(submission_path: Path, text: str) -> list[str]:
    """Check strict metadata requirements on 01_gemini_submission.md."""
    errs: list[str] = []
    if "post_review_action" not in text:
        errs.append(f"{submission_path}: missing required 'post_review_action' field.")
    if "review_range" not in text and "审查范围" not in text:
        errs.append(f"{submission_path}: missing required frozen 'review_range' field.")
    if "remote_tracking" not in text and "基线" not in text:
        errs.append(f"{submission_path}: missing 'remote_tracking' / baseline field.")
    return errs


def cmd_check(args: argparse.Namespace) -> int:
    repo_root = find_repo_root(args.repo_root)
    errors: list[str] = []
    warnings: list[str] = []
    checked_files: list[str] = []

    if args.files:
        for raw_file in args.files:
            path = ensure_within(repo_root, (repo_root / raw_file) if not Path(raw_file).is_absolute() else Path(raw_file))
            if not path.is_file():
                errors.append(f"Missing target file: {path}")
                continue
            text, fmt_errs = read_text_lf_strict(path)
            errors.extend(fmt_errs)
            if text is not None:
                errors.extend(check_markdown_links(path, text))
            checked_files.append(str(path))
    else:
        review_root = ensure_within(repo_root, repo_root / "out" / "codex_review")
        batch_name = args.batch
        if not batch_name:
            raise HelperError("Specify either --batch <name> or --files <path...>.")
        batch = validate_batch_name(batch_name)
        batch_dir = ensure_within(review_root, review_root / batch)
        if not batch_dir.is_dir():
            raise HelperError(f"Review batch directory not found: {batch_dir}")
        md_files = sorted(batch_dir.glob("*.md"))
        if not md_files:
            raise HelperError(f"No markdown files found in {batch_dir}")
        seq_errs, seq_warns = check_round_sequence(md_files, batch_dir)
        errors.extend(seq_errs)
        warnings.extend(seq_warns)
        for path in md_files:
            text, fmt_errs = read_text_lf_strict(path)
            errors.extend(fmt_errs)
            if text is not None:
                errors.extend(check_markdown_links(path, text))
                if args.strict_meta and path.name == "01_gemini_submission.md":
                    errors.extend(check_submission_metadata(path, text))
            checked_files.append(str(path))
        if args.check_latest:
            latest_path = ensure_within(review_root, review_root / "LATEST.md")
            if not latest_path.is_file():
                errors.append(f"Missing LATEST.md at {latest_path}")
            else:
                text, fmt_errs = read_text_lf_strict(latest_path)
                errors.extend(fmt_errs)
                if text is not None:
                    errors.extend(check_markdown_links(latest_path, text))
                    if batch not in text:
                        errors.append(f"{latest_path}: active batch '{batch}' is not referenced.")
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
    review_root = ensure_within(repo_root, repo_root / "out" / "codex_review")
    latest_path = review_root / "LATEST.md"

    _, head_short = resolve_commit(repo_root, "HEAD")
    branch = current_branch(repo_root)
    up_name, up_full = upstream_info(repo_root)
    dirty = bool(run_git(repo_root, "status", "--porcelain").stdout.strip())

    batches: list[dict[str, Any]] = []
    if review_root.is_dir():
        for entry in sorted(review_root.iterdir()):
            if entry.is_dir() and not entry.name.startswith("."):
                files = sorted(p.name for p in entry.glob("*.md"))
                batches.append({"batch": entry.name, "files": files})

    latest_excerpt = ""
    if latest_path.is_file():
        latest_excerpt = latest_path.read_text(encoding="utf-8", errors="replace").strip()

    payload = {
        "repo_root": str(repo_root),
        "branch": branch,
        "head": head_short,
        "upstream": f"{up_name}@{up_full[:8]}" if up_name and up_full else None,
        "dirty": dirty,
        "latest_exists": latest_path.is_file(),
        "batches": batches,
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
    p_init.add_argument("--force", action="store_true", help="Overwrite existing 01_gemini_submission.md")
    p_init.add_argument("--no-update-latest", action="store_true", help="Do not overwrite LATEST.md")
    p_init.add_argument("--repo-root", type=Path, default=None, help="Override repository root (for tests)")
    p_init.set_defaults(func=cmd_init)

    p_check = sub.add_parser("check", help="Validate review markdown encoding, LF, sequence, links, and line bounds")
    p_check.add_argument("--batch", default=None, help="Batch folder name under out/codex_review/")
    p_check.add_argument("--files", nargs="*", default=None, help="Explicit markdown files relative to repo root")
    p_check.add_argument("--check-latest", action="store_true", help="Also verify out/codex_review/LATEST.md")
    p_check.add_argument("--strict-meta", action="store_true", help="Require post_review_action and frozen range fields")
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
