"""Unit tests for qkeymapper-codex-review/scripts/review_helper.py."""

from __future__ import annotations

import contextlib
import io
import json
import os
from pathlib import Path
import shutil
import stat
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parent))
import review_helper  # noqa: E402


def _remove_readonly(func, path, exc_info):
    """Clear read-only attributes and remove Windows junctions safely during test cleanup."""
    if os.path.islink(path) or (
        getattr(os.lstat(path), "st_file_attributes", 0) & review_helper.FILE_ATTRIBUTE_REPARSE_POINT
    ):
        os.rmdir(path)
        return
    os.chmod(path, stat.S_IWRITE)
    func(path)


def _create_dir_junction(link_path: Path, target_path: Path) -> None:
    """Create a Windows NTFS directory junction without administrator privileges."""
    subprocess.run(
        ["cmd", "/c", "mklink", "/J", str(link_path), str(target_path)],
        capture_output=True,
        text=True,
        check=True,
    )


class TempGitRepo:
    """Self-cleaning isolated temporary workspace with outer dir + nested Git repo."""

    def __init__(self) -> None:
        self._outer_tmp = Path(tempfile.mkdtemp(prefix="qkm_review_outer_"))
        self.root = self._outer_tmp / "repo"
        self.root.mkdir()
        self._git("init", "-b", "feature/test")
        self._git("config", "user.name", "Tester")
        self._git("config", "user.email", "tester@example.com")
        (self.root / "src").mkdir()
        (self.root / "docs").mkdir()
        (self.root / ".agents" / "skills" / "qkeymapper-codex-review" / "references").mkdir(parents=True)
        (self.root / ".agents" / "skills" / "qkeymapper-codex-review" / "SKILL.md").write_text(
            "# Skill\nLine 2\n", encoding="utf-8", newline="\n"
        )
        (
            self.root
            / ".agents"
            / "skills"
            / "qkeymapper-codex-review"
            / "references"
            / "mcp-handoff-checklist.md"
        ).write_text("# Checklist\n", encoding="utf-8", newline="\n")
        (self.root / ".gitignore").write_text("out/\n", encoding="utf-8", newline="\n")
        (self.root / "src" / "main.cpp").write_text(
            "int main() {\n    return 0;\n}\n", encoding="utf-8", newline="\n"
        )
        self._git("add", ".")
        self._git("commit", "-m", "chore: seed commit")
        self.initial_sha = self._rev_parse("HEAD")

        # Second pushed baseline commit so initial_sha..upstream_sha is a non-empty pushed range
        (self.root / "src" / "base.cpp").write_text("// base\n", encoding="utf-8", newline="\n")
        self._git("add", "src/base.cpp")
        self._git("commit", "-m", "chore: upstream baseline")
        self.upstream_sha = self._rev_parse("HEAD")
        self._git("config", "remote.origin.url", "https://example.invalid/repo.git")
        self._git("config", "remote.origin.fetch", "+refs/heads/*:refs/remotes/origin/*")
        self._git("update-ref", "refs/remotes/origin/feature/test", self.upstream_sha)
        self._git("branch", "--set-upstream-to=origin/feature/test", "feature/test")

        # Deferred commit (ahead of upstream, before the reviewed range base)
        (self.root / "src" / "deferred.cpp").write_text("// deferred\n", encoding="utf-8", newline="\n")
        self._git("add", "src/deferred.cpp")
        self._git("commit", "-m", "feat(mcp): deferred commit")
        self.deferred_sha = self._rev_parse("HEAD")

        # Two commits inside the reviewed range
        (self.root / "src" / "main.cpp").write_text(
            "int main() {\n    int x = 1;\n    return x - 1;\n}\n", encoding="utf-8", newline="\n"
        )
        self._git("add", "src/main.cpp")
        self._git("commit", "-m", "feat(ui): code change")
        self.code_sha = self._rev_parse("HEAD")

        (self.root / "docs" / "note.md").write_text("# Notes\nline2\n", encoding="utf-8", newline="\n")
        self._git("add", "docs/note.md")
        self._git("commit", "-m", "docs(ui): update note")
        self.doc_sha = self._rev_parse("HEAD")

    def _git(self, *args: str) -> str:
        proc = subprocess.run(
            ["git", "--no-optional-locks", *args],
            cwd=str(self.root),
            capture_output=True,
            text=True,
            encoding="utf-8",
            check=True,
        )
        return proc.stdout.strip()

    def _rev_parse(self, ref: str) -> str:
        return self._git("rev-parse", "--verify", ref)

    def cleanup(self) -> None:
        if self._outer_tmp.exists():
            shutil.rmtree(self._outer_tmp, onerror=_remove_readonly)


def run_cli(*args: str) -> tuple[int, str, str]:
    out_buf = io.StringIO()
    err_buf = io.StringIO()
    with contextlib.redirect_stdout(out_buf), contextlib.redirect_stderr(err_buf):
        rc = review_helper.main(list(args))
    return rc, out_buf.getvalue(), err_buf.getvalue()


def _write_reviewer_md(
    path: Path,
    *,
    base_sha: str,
    code_sha: str,
    doc_sha: str,
    verdict: str,
    post_action: str = "pause_for_user",
    profile: str = "general",
    omit_code_head: bool = False,
    body_extra: str = "",
) -> None:
    """Write a canonical Reviewer markdown fixture for strict endpoint and verdict tests."""
    code_frag = "" if omit_code_head else f"，`code_head={code_sha[:8]}`"
    extra_block = f"\n{body_extra.rstrip()}\n" if body_extra else ""
    path.write_text(
        "# QKeyMapper 本地提交复审\n\n"
        "## 1. 复审元信息\n\n"
        f"- 冻结范围 `{base_sha[:8]}..{doc_sha[:8]}`{code_frag}，`review_profile={profile}`，`post_review_action={post_action}`。\n\n"
        "## 2. 总体结论\n\n"
        f"**{verdict}。** 本轮核对完成。\n"
        f"{extra_block}",
        encoding="utf-8",
        newline="\n",
    )


class ReviewHelperTests(unittest.TestCase):
    def setUp(self) -> None:
        self.repo = TempGitRepo()
        self.addCleanup(self.repo.cleanup)

    def test_init_and_strict_check_with_deferred_commits(self) -> None:
        rc, out, err = run_cli(
            "init",
            "--batch",
            "20261127_ui_fix",
            "--base",
            self.repo.deferred_sha,
            "--head",
            self.repo.doc_sha,
            "--code-head",
            self.repo.code_sha,
            "--post-action",
            "pause_for_user",
            "--profile",
            "general",
            "--title",
            "测试局部 UI 修复",
            "--repo-root",
            str(self.repo.root),
        )
        self.assertEqual(rc, 0, err)
        meta = json.loads(out)
        self.assertEqual(meta["commits"], 2)
        self.assertEqual(meta["post_review_action"], "pause_for_user")

        sub_path = self.repo.root / "out" / "codex_review" / "20261127_ui_fix" / "01_gemini_submission.md"
        latest_path = self.repo.root / "out" / "codex_review" / "LATEST.md"
        self.assertTrue(sub_path.is_file())
        self.assertTrue(latest_path.is_file())

        sub_raw = sub_path.read_bytes()
        self.assertFalse(sub_raw.startswith(b"\xef\xbb\xbf"))
        self.assertNotIn(b"\r", sub_raw)
        sub_text = sub_raw.decode("utf-8")
        self.assertIn(self.repo.upstream_sha[:8], sub_text)
        self.assertIn("延后至后续批次回顾", sub_text)
        self.assertIn(self.repo.code_sha[:8], sub_text)
        self.assertIn(self.repo.doc_sha[:8], sub_text)
        self.assertIn("pause_for_user", sub_text)

        rc2, out2, _ = run_cli(
            "check",
            "--batch",
            "20261127_ui_fix",
            "--check-latest",
            "--strict-meta",
            "--json",
            "--repo-root",
            str(self.repo.root),
        )
        self.assertEqual(rc2, 0, out2)
        check_res = json.loads(out2)
        self.assertTrue(check_res["ok"])
        self.assertEqual(check_res["error_count"], 0)

        # Exclusive creation refuses duplicate init and leaves 01 intact
        rc_dup, _, err_dup = run_cli(
            "init",
            "--batch",
            "20261127_ui_fix",
            "--base",
            self.repo.deferred_sha,
            "--head",
            self.repo.doc_sha,
            "--repo-root",
            str(self.repo.root),
        )
        self.assertNotEqual(rc_dup, 0)
        self.assertIn("already exists", err_dup)
        self.assertEqual(sub_path.read_bytes(), sub_raw)

    def test_write_and_status_guards_junction_and_hardlink(self) -> None:
        # 1. Junction at out/ itself -> status must reject BEFORE touching out/codex_review
        outside_dir = self.repo._outer_tmp / "outside_target"
        outside_dir.mkdir()
        (outside_dir / "codex_review").mkdir()
        out_dir = self.repo.root / "out"
        _create_dir_junction(out_dir, outside_dir)
        rc_st_out, _, err_st_out = run_cli("status", "--repo-root", str(self.repo.root))
        self.assertNotEqual(rc_st_out, 0)
        self.assertIn("reparse", err_st_out.lower())
        os.rmdir(out_dir)

        # 2. Junction at out/codex_review -> src/ rejected cleanly on init & status
        out_dir.mkdir()
        review_dir = out_dir / "codex_review"
        _create_dir_junction(review_dir, self.repo.root / "src")
        rc_junc, _, err_junc = run_cli(
            "init",
            "--batch",
            "20261127_junc",
            "--base",
            self.repo.deferred_sha,
            "--head",
            self.repo.doc_sha,
            "--repo-root",
            str(self.repo.root),
        )
        self.assertNotEqual(rc_junc, 0)
        self.assertIn("reparse", err_junc.lower())
        self.assertFalse((self.repo.root / "src" / "20261127_junc").exists())
        os.rmdir(review_dir)

        # 3. Hardlinked LATEST.md rejected on init, status, and check --check-latest
        review_dir.mkdir()
        tracked_canary = self.repo.root / "docs" / "note.md"
        latest_path = review_dir / "LATEST.md"
        os.link(tracked_canary, latest_path)
        rc_hl, _, err_hl = run_cli(
            "init",
            "--batch",
            "20261127_hardlink",
            "--base",
            self.repo.deferred_sha,
            "--head",
            self.repo.doc_sha,
            "--repo-root",
            str(self.repo.root),
        )
        self.assertNotEqual(rc_hl, 0)
        self.assertIn("hard-linked", err_hl.lower())
        self.assertFalse((review_dir / "20261127_hardlink").exists())

        rc_hl_st, out_hl_st, err_hl_st = run_cli("status", "--repo-root", str(self.repo.root))
        self.assertNotEqual(rc_hl_st, 0)
        self.assertIn("hard-linked", err_hl_st.lower())
        self.assertNotIn("line2", out_hl_st)
        self.assertEqual(tracked_canary.read_text(encoding="utf-8"), "# Notes\nline2\n")

    def test_init_partial_failure_recovery_with_sync_latest(self) -> None:
        review_dir = self.repo.root / "out" / "codex_review"
        review_dir.mkdir(parents=True, exist_ok=True)
        latest_path = review_dir / "LATEST.md"
        latest_path.write_text("# Old Latest\n", encoding="utf-8", newline="\n")

        with mock.patch.object(review_helper, "write_atomic_lf", side_effect=OSError("locked")):
            rc_fail, _, err_fail = run_cli(
                "init",
                "--batch",
                "20261127_recover",
                "--base",
                self.repo.deferred_sha,
                "--head",
                self.repo.doc_sha,
                "--repo-root",
                str(self.repo.root),
            )
        self.assertNotEqual(rc_fail, 0)
        self.assertIn("sync-latest --batch 20261127_recover", err_fail)
        self.assertTrue((review_dir / "20261127_recover" / "01_gemini_submission.md").is_file())
        self.assertEqual(latest_path.read_text(encoding="utf-8"), "# Old Latest\n")

        # Retry via sync-latest preserves 01 and atomically updates LATEST.md
        rc_sync, out_sync, err_sync = run_cli(
            "sync-latest",
            "--batch",
            "20261127_recover",
            "--repo-root",
            str(self.repo.root),
        )
        self.assertEqual(rc_sync, 0, err_sync)
        self.assertEqual(json.loads(out_sync)["status"], "WAITING_R1_REVIEW")
        rc_chk, out_chk, _ = run_cli(
            "check",
            "--batch",
            "20261127_recover",
            "--check-latest",
            "--strict-meta",
            "--json",
            "--repo-root",
            str(self.repo.root),
        )
        self.assertEqual(rc_chk, 0, out_chk)

    def test_guards_dirty_tree_bad_batch_range_and_nonempty_pushed(self) -> None:
        for bad_batch in ("../escape", "a/b", ".hidden", "bad\\name"):
            rc, _, _ = run_cli(
                "init",
                "--batch",
                bad_batch,
                "--base",
                self.repo.upstream_sha,
                "--head",
                self.repo.doc_sha,
                "--repo-root",
                str(self.repo.root),
            )
            self.assertNotEqual(rc, 0, bad_batch)

        # Reject NON-EMPTY already-pushed range initial_sha..upstream_sha explicitly via "already pushed"
        rc_pushed, _, err_pushed = run_cli(
            "init",
            "--batch",
            "20261127_pushed",
            "--base",
            self.repo.initial_sha,
            "--head",
            self.repo.upstream_sha,
            "--repo-root",
            str(self.repo.root),
        )
        self.assertNotEqual(rc_pushed, 0)
        self.assertIn("already pushed", err_pushed.lower())

        # Reject swapped base/head
        rc_swap, _, _ = run_cli(
            "init",
            "--batch",
            "20261127_swapped",
            "--base",
            self.repo.doc_sha,
            "--head",
            self.repo.deferred_sha,
            "--repo-root",
            str(self.repo.root),
        )
        self.assertNotEqual(rc_swap, 0)

        # Reject wrong branch head not on current branch
        self.repo._git("checkout", "-b", "other-branch")
        (self.repo.root / "src" / "other.cpp").write_text("// other\n", encoding="utf-8", newline="\n")
        self.repo._git("add", "src/other.cpp")
        self.repo._git("commit", "-m", "feat: other branch commit")
        other_sha = self.repo._rev_parse("HEAD")
        self.repo._git("checkout", "feature/test")
        rc_other, _, err_other = run_cli(
            "init",
            "--batch",
            "20261127_other",
            "--base",
            self.repo.deferred_sha,
            "--head",
            other_sha,
            "--repo-root",
            str(self.repo.root),
        )
        self.assertNotEqual(rc_other, 0)
        self.assertIn("not an ancestor of current HEAD", err_other)

        # Pinning earlier commit on current branch AFTER current HEAD advanced is allowed
        (self.repo.root / "src" / "later.cpp").write_text("// later\n", encoding="utf-8", newline="\n")
        self.repo._git("add", "src/later.cpp")
        self.repo._git("commit", "-m", "feat: later commit on same branch")
        rc_earlier, _, err_earlier = run_cli(
            "init",
            "--batch",
            "20261127_pinned_earlier",
            "--base",
            self.repo.deferred_sha,
            "--head",
            self.repo.doc_sha,
            "--repo-root",
            str(self.repo.root),
        )
        self.assertEqual(rc_earlier, 0, err_earlier)

        # Reject dirty tree without --allow-dirty
        (self.repo.root / "src" / "main.cpp").write_text("// dirty\n", encoding="utf-8", newline="\n")
        rc_dirty, _, err_dirty = run_cli(
            "init",
            "--batch",
            "20261127_dirty",
            "--base",
            self.repo.upstream_sha,
            "--head",
            self.repo.doc_sha,
            "--repo-root",
            str(self.repo.root),
        )
        self.assertNotEqual(rc_dirty, 0)
        self.assertIn("not clean", err_dirty)

    def test_handoff_profile_trigger_including_non_ascii_paths(self) -> None:
        handoff_dir = self.repo.root / "handoff"
        handoff_dir.mkdir(exist_ok=True)
        (handoff_dir / "README.md").write_text("# Index\n", encoding="utf-8", newline="\n")
        self.repo._git("add", "handoff/README.md")
        self.repo._git("commit", "-m", "docs: add top handoff README")
        nav_sha = self.repo._rev_parse("HEAD")
        rc_nav, _, err_nav = run_cli(
            "init",
            "--batch",
            "20261127_nav",
            "--base",
            self.repo.doc_sha,
            "--head",
            nav_sha,
            "--profile",
            "general",
            "--repo-root",
            str(self.repo.root),
        )
        self.assertEqual(rc_nav, 0, err_nav)

        # Non-ASCII Chinese path under handoff/<task>/ would be C-quoted by git without -z!
        task_dir = handoff_dir / "中文任务目录"
        task_dir.mkdir()
        (task_dir / "主计划.md").write_text("# 中文主计划\n", encoding="utf-8", newline="\n")
        self.repo._git("add", "handoff/中文任务目录/主计划.md")
        self.repo._git("commit", "-m", "docs(mcp): update non-ascii task plan")
        plan_sha = self.repo._rev_parse("HEAD")
        rc_gen, _, err_gen = run_cli(
            "init",
            "--batch",
            "20261127_handoff_nonascii",
            "--base",
            nav_sha,
            "--head",
            plan_sha,
            "--profile",
            "general",
            "--repo-root",
            str(self.repo.root),
        )
        self.assertNotEqual(rc_gen, 0)
        self.assertIn("handoff-plan", err_gen)

        rc_hp, _, err_hp = run_cli(
            "init",
            "--batch",
            "20261127_handoff_nonascii",
            "--base",
            nav_sha,
            "--head",
            plan_sha,
            "--profile",
            "handoff-plan",
            "--repo-root",
            str(self.repo.root),
        )
        self.assertEqual(rc_hp, 0, err_hp)

        # Rename/move out of handoff/<task>/ must still trigger handoff-plan via --no-renames
        self.repo._git("config", "diff.renames", "copies")
        self.repo._git("mv", "handoff/中文任务目录/主计划.md", "docs/archived_plan.md")
        self.repo._git("commit", "-m", "docs: move task plan out of handoff")
        mv_sha = self.repo._rev_parse("HEAD")
        changed_mv = review_helper.changed_files_in_range(self.repo.root, plan_sha, mv_sha)
        self.assertIn("handoff/中文任务目录/主计划.md", changed_mv)
        self.assertIn("docs/archived_plan.md", changed_mv)

        rc_mv_gen, _, err_mv_gen = run_cli(
            "init",
            "--batch",
            "20261127_handoff_mv",
            "--base",
            plan_sha,
            "--head",
            mv_sha,
            "--profile",
            "general",
            "--repo-root",
            str(self.repo.root),
        )
        self.assertNotEqual(rc_mv_gen, 0)
        self.assertIn("handoff-plan", err_mv_gen)

        rc_mv_hp, _, err_mv_hp = run_cli(
            "init",
            "--batch",
            "20261127_handoff_mv",
            "--base",
            plan_sha,
            "--head",
            mv_sha,
            "--profile",
            "handoff-plan",
            "--repo-root",
            str(self.repo.root),
        )
        self.assertEqual(rc_mv_hp, 0, err_mv_hp)

    def test_check_read_boundaries_response_binding_and_flag_exclusivity(self) -> None:
        rc, _, err = run_cli(
            "init",
            "--batch",
            "20261127_links",
            "--base",
            self.repo.deferred_sha,
            "--head",
            self.repo.doc_sha,
            "--repo-root",
            str(self.repo.root),
        )
        self.assertEqual(rc, 0, err)

        # Reject mixing --files with --batch / --strict-meta / --check-latest
        rc_mix, _, err_mix = run_cli(
            "check",
            "--files",
            "docs/note.md",
            "--batch",
            "20261127_links",
            "--strict-meta",
            "--check-latest",
            "--repo-root",
            str(self.repo.root),
        )
        self.assertNotEqual(rc_mix, 0)
        self.assertIn("cannot be combined", err_mix)

        # Reject mid-path drive/colon in --files BEFORE lstat
        rc_files_colon, out_files_colon, _ = run_cli(
            "check",
            "--files",
            "docs/C:evil.md",
            "--json",
            "--repo-root",
            str(self.repo.root),
        )
        self.assertEqual(rc_files_colon, 1)
        self.assertIn("colon", "\n".join(json.loads(out_files_colon)["errors"]).lower())

        batch_dir = self.repo.root / "out" / "codex_review" / "20261127_links"
        # Outside secret strictly inside our isolated _outer_tmp (never shared TEMP)
        outside_file = self.repo._outer_tmp / "outside_secret.txt"
        outside_file.write_text("secret\n", encoding="utf-8", newline="\n")

        junc_inside = self.repo.root / "docs" / "junc_src"
        _create_dir_junction(junc_inside, self.repo.root / "src")

        # Also create a hardlinked markdown target inside docs/ to test check_markdown_links hardlink block
        hl_doc = self.repo.root / "docs" / "hardlinked_note.md"
        os.link(self.repo.root / "docs" / "note.md", hl_doc)

        r2_path = batch_dir / "02_codex_review_r1.md"
        r2_path.write_text(
            "# Review\n"
            "[ok](../../../src/main.cpp#L1-L3)\n"
            "[web](https://example.com/foo)\n"
            "Inline `[ignored](../../../does/not/exist.txt)`\n"
            "```markdown\n[ignored_fence](../../../nope.md)\n```\n"
            "[empty](   )\n"
            "[abs](C:/Windows/win.ini)\n"
            "[mid_drive](../../../docs/C:evil.txt)\n"
            "[unc](\\\\server\\share\\x.txt)\n"
            "[escape](../../../../outside_secret.txt)\n"
            "[junc_link](../../../docs/junc_src/main.cpp)\n"
            "[hl_link](../../../docs/hardlinked_note.md#L1)\n"
            "[broken](../../../missing_file.cpp)\n"
            "[oob](../../../src/main.cpp#L1-L999)\n",
            encoding="utf-8",
            newline="\n",
        )
        rc_bad, out_bad, _ = run_cli(
            "check",
            "--batch",
            "20261127_links",
            "--json",
            "--repo-root",
            str(self.repo.root),
        )
        self.assertEqual(rc_bad, 1, out_bad)
        joined = "\n".join(json.loads(out_bad)["errors"])
        self.assertIn("empty link target", joined)
        self.assertIn("C:/Windows/win.ini", joined)
        self.assertIn("docs/C:evil.txt", joined)
        self.assertIn("\\\\server\\share\\x.txt", joined)
        self.assertIn("escapes repository root", joined)
        self.assertIn("reparse", joined.lower())
        self.assertIn("hard-linked", joined.lower())
        self.assertIn("missing_file.cpp", joined)
        self.assertIn("L1-L999", joined)
        self.assertNotIn("does/not/exist.txt", joined)
        self.assertNotIn("nope.md", joined)

        _write_reviewer_md(
            r2_path,
            base_sha=self.repo.deferred_sha,
            code_sha=self.repo.doc_sha,
            doc_sha=self.repo.doc_sha,
            verdict="CHANGES_REQUESTED",
            body_extra="## 3. 发现清单\n\n- [ok](../../../src/main.cpp#L1-L3)",
        )

        # Duplicate post_review_action row in 01_gemini_submission.md must fail check
        sub = batch_dir / "01_gemini_submission.md"
        orig_sub = sub.read_text(encoding="utf-8")
        sub.write_text(
            orig_sub.replace(
                "| `post_review_action` | `pause_for_user` |",
                "| `post_review_action` | `pause_for_user` |\n| `post_review_action` | `continue_after_pass` |",
            ),
            encoding="utf-8",
            newline="\n",
        )
        rc_dup_key, out_dup_key, _ = run_cli(
            "check",
            "--batch",
            "20261127_links",
            "--strict-meta",
            "--json",
            "--repo-root",
            str(self.repo.root),
        )
        self.assertEqual(rc_dup_key, 1)
        self.assertIn("duplicate metadata row 'post_review_action'", "\n".join(json.loads(out_dup_key)["errors"]))
        sub.write_text(orig_sub, encoding="utf-8", newline="\n")

        # Create a fix commit and 03_gemini_response_r1.md, verify LATEST binding + mismatch rejections
        (self.repo.root / "src" / "fix.cpp").write_text("// fix\n", encoding="utf-8", newline="\n")
        self.repo._git("add", "src/fix.cpp")
        self.repo._git("commit", "-m", "fix(ui): address review")
        fix_sha = self.repo._rev_parse("HEAD")

        r3_path = batch_dir / "03_gemini_response_r1.md"
        r3_path.write_text(
            "# Response r1\n\n"
            f"- **前置基线 → 新增本地提交**：`{self.repo.doc_sha[:8]}..{fix_sha[:8]}`\n"
            "- **`post_review_action`**：`pause_for_user`\n",
            encoding="utf-8",
            newline="\n",
        )
        # While LATEST still points to 01_gemini_submission.md, check --check-latest --strict-meta must fail!
        rc_stale_lat, out_stale_lat, _ = run_cli(
            "check",
            "--batch",
            "20261127_links",
            "--check-latest",
            "--strict-meta",
            "--json",
            "--repo-root",
            str(self.repo.root),
        )
        self.assertEqual(rc_stale_lat, 1)
        stale_errs = "\n".join(json.loads(out_stale_lat)["errors"])
        self.assertIn("03_gemini_response_r1.md", stale_errs)

        # sync-latest updates LATEST cleanly to 03 + fix_sha, then strict check passes!
        rc_sync2, _, err_sync2 = run_cli(
            "sync-latest",
            "--batch",
            "20261127_links",
            "--repo-root",
            str(self.repo.root),
        )
        self.assertEqual(rc_sync2, 0, err_sync2)
        rc_ok2, out_ok2, _ = run_cli(
            "check",
            "--batch",
            "20261127_links",
            "--check-latest",
            "--strict-meta",
            "--json",
            "--repo-root",
            str(self.repo.root),
        )
        self.assertEqual(rc_ok2, 0, out_ok2)

        # Mismatched review_profile in LATEST must fail
        latest_path = self.repo.root / "out" / "codex_review" / "LATEST.md"
        latest_ok = latest_path.read_text(encoding="utf-8")
        latest_path.write_text(
            latest_ok.replace("`general`", "`handoff-plan`"), encoding="utf-8", newline="\n"
        )
        rc_prof, out_prof, _ = run_cli(
            "check",
            "--batch",
            "20261127_links",
            "--check-latest",
            "--strict-meta",
            "--json",
            "--repo-root",
            str(self.repo.root),
        )
        self.assertEqual(rc_prof, 1)
        self.assertIn("review_profile 'handoff-plan' does not match submission 'general'", "\n".join(json.loads(out_prof)["errors"]))

    def test_frozen_code_head_binding_across_responses_and_sync(self) -> None:
        rc, _, err = run_cli(
            "init",
            "--batch",
            "20261127_codehead",
            "--base",
            self.repo.deferred_sha,
            "--head",
            self.repo.doc_sha,
            "--code-head",
            self.repo.code_sha,
            "--repo-root",
            str(self.repo.root),
        )
        self.assertEqual(rc, 0, err)

        batch_dir = self.repo.root / "out" / "codex_review" / "20261127_codehead"
        latest_path = self.repo.root / "out" / "codex_review" / "LATEST.md"

        # sync-latest must no longer expose --code-head override
        with contextlib.redirect_stderr(io.StringIO()), self.assertRaises(SystemExit) as cm:
            review_helper.main(
                [
                    "sync-latest",
                    "--batch",
                    "20261127_codehead",
                    "--code-head",
                    self.repo.code_sha,
                    "--repo-root",
                    str(self.repo.root),
                ]
            )
        self.assertNotEqual(cm.exception.code, 0)

        _write_reviewer_md(
            batch_dir / "02_codex_review_r1.md",
            base_sha=self.repo.deferred_sha,
            code_sha=self.repo.code_sha,
            doc_sha=self.repo.doc_sha,
            verdict="CHANGES_REQUESTED",
        )

        # r1 response defaults code_head to fix1_sha
        (self.repo.root / "src" / "fix1.cpp").write_text("// fix1\n", encoding="utf-8", newline="\n")
        self.repo._git("add", "src/fix1.cpp")
        self.repo._git("commit", "-m", "fix(ui): fix1")
        fix1_sha = self.repo._rev_parse("HEAD")

        r3_path = batch_dir / "03_gemini_response_r1.md"
        r3_path.write_text(
            "# Response r1\n\n"
            f"- **前置基线 → 新增本地提交**：`{self.repo.doc_sha[:8]}..{fix1_sha[:8]}`\n"
            "- **`post_review_action`**：`pause_for_user`\n",
            encoding="utf-8",
            newline="\n",
        )
        rc_s1, out_s1, err_s1 = run_cli(
            "sync-latest", "--batch", "20261127_codehead", "--repo-root", str(self.repo.root)
        )
        self.assertEqual(rc_s1, 0, err_s1)
        self.assertEqual(json.loads(out_s1)["code_head"], fix1_sha[:8])

        # Tampering LATEST code_head back to initial code_sha or doc_sha must FAIL strict check
        latest_r3 = latest_path.read_text(encoding="utf-8")
        latest_path.write_text(
            latest_r3.replace(f"代码终点 `{fix1_sha[:8]}`", f"代码终点 `{self.repo.code_sha[:8]}`"),
            encoding="utf-8",
            newline="\n",
        )
        rc_drift, out_drift, _ = run_cli(
            "check",
            "--batch",
            "20261127_codehead",
            "--check-latest",
            "--strict-meta",
            "--json",
            "--repo-root",
            str(self.repo.root),
        )
        self.assertEqual(rc_drift, 1)
        self.assertIn("frozen code_head", "\n".join(json.loads(out_drift)["errors"]))
        latest_path.write_text(latest_r3, encoding="utf-8", newline="\n")

        _write_reviewer_md(
            batch_dir / "04_codex_recheck_r1.md",
            base_sha=self.repo.deferred_sha,
            code_sha=fix1_sha,
            doc_sha=fix1_sha,
            verdict="CHANGES_REQUESTED",
        )

        # r2 fix has code commit fix2_code_sha + doc-only tail commit fix2_doc_sha
        (self.repo.root / "src" / "fix2.cpp").write_text("// fix2\n", encoding="utf-8", newline="\n")
        self.repo._git("add", "src/fix2.cpp")
        self.repo._git("commit", "-m", "fix(ui): fix2")
        fix2_code_sha = self.repo._rev_parse("HEAD")

        (self.repo.root / "docs" / "fix2.md").write_text("# Fix2 Doc\n", encoding="utf-8", newline="\n")
        self.repo._git("add", "docs/fix2.md")
        self.repo._git("commit", "-m", "docs(ui): explain fix2")
        fix2_doc_sha = self.repo._rev_parse("HEAD")

        r5_path = batch_dir / "05_gemini_response_r2.md"
        # Regression check: code_head before previous response code_head (self.repo.code_sha < fix1_sha) fails
        r5_path.write_text(
            "# Response r2\n\n"
            f"- **前置基线 → 新增本地提交**：`{fix1_sha[:8]}..{fix2_doc_sha[:8]}`\n"
            f"- **`code_head`**：`{self.repo.code_sha[:8]}`\n"
            "- **`post_review_action`**：`pause_for_user`\n",
            encoding="utf-8",
            newline="\n",
        )
        rc_reg, out_reg, _ = run_cli(
            "check",
            "--batch",
            "20261127_codehead",
            "--strict-meta",
            "--json",
            "--repo-root",
            str(self.repo.root),
        )
        self.assertEqual(rc_reg, 1)
        self.assertIn("regresses", "\n".join(json.loads(out_reg)["errors"]))

        # Duplicate code_head lines in 05 fail
        r5_path.write_text(
            "# Response r2\n\n"
            f"- **前置基线 → 新增本地提交**：`{fix1_sha[:8]}..{fix2_doc_sha[:8]}`\n"
            f"- **`code_head`**：`{fix2_code_sha[:8]}`\n"
            f"- **`code_head`**：`{fix2_doc_sha[:8]}`\n"
            "- **`post_review_action`**：`pause_for_user`\n",
            encoding="utf-8",
            newline="\n",
        )
        rc_dup_c, out_dup_c, _ = run_cli(
            "check",
            "--batch",
            "20261127_codehead",
            "--strict-meta",
            "--json",
            "--repo-root",
            str(self.repo.root),
        )
        self.assertEqual(rc_dup_c, 1)
        self.assertIn("at most 1", "\n".join(json.loads(out_dup_c)["errors"]))

        # Valid explicit code_head in 05 succeeds across sync-latest and strict check
        r5_path.write_text(
            "# Response r2\n\n"
            f"- **前置基线 → 新增本地提交**：`{fix1_sha[:8]}..{fix2_doc_sha[:8]}`\n"
            f"- **`code_head`**：`{fix2_code_sha[:8]}`\n"
            "- **`post_review_action`**：`pause_for_user`\n",
            encoding="utf-8",
            newline="\n",
        )
        rc_s2, out_s2, err_s2 = run_cli(
            "sync-latest", "--batch", "20261127_codehead", "--repo-root", str(self.repo.root)
        )
        self.assertEqual(rc_s2, 0, err_s2)
        s2_meta = json.loads(out_s2)
        self.assertEqual(s2_meta["code_head"], fix2_code_sha[:8])
        self.assertEqual(s2_meta["doc_head"], fix2_doc_sha[:8])
        self.assertEqual(s2_meta["status"], "WAITING_R2_RECHECK")

        rc_ok, out_ok, _ = run_cli(
            "check",
            "--batch",
            "20261127_codehead",
            "--check-latest",
            "--strict-meta",
            "--json",
            "--repo-root",
            str(self.repo.root),
        )
        self.assertEqual(rc_ok, 0, out_ok)

        # Tampering LATEST code_head to fix2_doc_sha (when 05 froze fix2_code_sha) fails
        latest_r5 = latest_path.read_text(encoding="utf-8")
        latest_path.write_text(
            latest_r5.replace(f"代码终点 `{fix2_code_sha[:8]}`", f"代码终点 `{fix2_doc_sha[:8]}`"),
            encoding="utf-8",
            newline="\n",
        )
        rc_drift2, out_drift2, _ = run_cli(
            "check",
            "--batch",
            "20261127_codehead",
            "--check-latest",
            "--strict-meta",
            "--json",
            "--repo-root",
            str(self.repo.root),
        )
        self.assertEqual(rc_drift2, 1)
        self.assertIn("frozen code_head", "\n".join(json.loads(out_drift2)["errors"]))

    def test_status_and_reviewer_verdict_binding(self) -> None:
        rc, _, err = run_cli(
            "init",
            "--batch",
            "20261127_verdict",
            "--base",
            self.repo.deferred_sha,
            "--head",
            self.repo.doc_sha,
            "--repo-root",
            str(self.repo.root),
        )
        self.assertEqual(rc, 0, err)

        batch_dir = self.repo.root / "out" / "codex_review" / "20261127_verdict"
        latest_path = self.repo.root / "out" / "codex_review" / "LATEST.md"

        _write_reviewer_md(
            batch_dir / "02_codex_review_r1.md",
            base_sha=self.repo.deferred_sha,
            code_sha=self.repo.doc_sha,
            doc_sha=self.repo.doc_sha,
            verdict="CHANGES_REQUESTED",
        )
        (self.repo.root / "src" / "vfix.cpp").write_text("// vfix\n", encoding="utf-8", newline="\n")
        self.repo._git("add", "src/vfix.cpp")
        self.repo._git("commit", "-m", "fix(ui): vfix")
        vfix_sha = self.repo._rev_parse("HEAD")

        (batch_dir / "03_gemini_response_r1.md").write_text(
            "# Response r1\n\n"
            f"- **前置基线 → 新增本地提交**：`{self.repo.doc_sha[:8]}..{vfix_sha[:8]}`\n"
            "- **`post_review_action`**：`pause_for_user`\n",
            encoding="utf-8",
            newline="\n",
        )

        # At 01..03 (no 04 yet), sync-latest must refuse PASS / CHANGES_REQUESTED / wrong WAITING round
        for bad_status in ("PASS", "CHANGES_REQUESTED", "WAITING_R2_RECHECK"):
            rc_bad_st, _, err_bad_st = run_cli(
                "sync-latest",
                "--batch",
                "20261127_verdict",
                "--status",
                bad_status,
                "--repo-root",
                str(self.repo.root),
            )
            self.assertNotEqual(rc_bad_st, 0, bad_status)
            self.assertIn("conflicts with verified batch state", err_bad_st)

        # Default sync-latest derives WAITING_R1_RECHECK and passes strict check
        rc_w1, out_w1, err_w1 = run_cli(
            "sync-latest", "--batch", "20261127_verdict", "--repo-root", str(self.repo.root)
        )
        self.assertEqual(rc_w1, 0, err_w1)
        self.assertEqual(json.loads(out_w1)["status"], "WAITING_R1_RECHECK")

        # Tampering LATEST status to PASS or dropping Codex pointer must FAIL strict check
        orig_lat = latest_path.read_text(encoding="utf-8")
        latest_path.write_text(
            orig_lat.replace("**WAITING_R1_RECHECK**", "**PASS**"), encoding="utf-8", newline="\n"
        )
        rc_tamper_st, out_tamper_st, _ = run_cli(
            "check",
            "--batch",
            "20261127_verdict",
            "--check-latest",
            "--strict-meta",
            "--json",
            "--repo-root",
            str(self.repo.root),
        )
        self.assertEqual(rc_tamper_st, 1)
        self.assertIn("conflicts with verified batch state", "\n".join(json.loads(out_tamper_st)["errors"]))

        latest_path.write_text(
            orig_lat.replace(
                "20261127_verdict/02_codex_review_r1.md", "20261127_verdict/99_wrong.md"
            ),
            encoding="utf-8",
            newline="\n",
        )
        rc_tamper_cx, _, _ = run_cli(
            "check",
            "--batch",
            "20261127_verdict",
            "--check-latest",
            "--strict-meta",
            "--json",
            "--repo-root",
            str(self.repo.root),
        )
        self.assertEqual(rc_tamper_cx, 1)

        # Doc-only tail commit for 04 test (explicit code_head != doc_head)
        _write_reviewer_md(
            batch_dir / "04_codex_recheck_r1.md",
            base_sha=self.repo.deferred_sha,
            code_sha=vfix_sha,
            doc_sha=vfix_sha,
            verdict="CHANGES_REQUESTED",
        )
        (self.repo.root / "src" / "vfix2.cpp").write_text("// vfix2\n", encoding="utf-8", newline="\n")
        self.repo._git("add", "src/vfix2.cpp")
        self.repo._git("commit", "-m", "fix(ui): vfix2")
        vfix2_code = self.repo._rev_parse("HEAD")
        (self.repo.root / "docs" / "vfix2.md").write_text("# vfix2 doc\n", encoding="utf-8", newline="\n")
        self.repo._git("add", "docs/vfix2.md")
        self.repo._git("commit", "-m", "docs(ui): vfix2 doc")
        vfix2_doc = self.repo._rev_parse("HEAD")

        (batch_dir / "05_gemini_response_r2.md").write_text(
            "# Response r2\n\n"
            f"- **前置基线 → 新增本地提交**：`{vfix_sha[:8]}..{vfix2_doc[:8]}`\n"
            f"- **`code_head`**：`{vfix2_code[:8]}`\n"
            "- **`post_review_action`**：`pause_for_user`\n",
            encoding="utf-8",
            newline="\n",
        )

        # 06 with stale range or missing code_head when code_head != doc_head must be rejected
        r6_path = batch_dir / "06_codex_recheck_r2.md"
        _write_reviewer_md(
            r6_path,
            base_sha=self.repo.deferred_sha,
            code_sha=vfix2_code,
            doc_sha=vfix_sha,
            verdict="PASS",
        )
        rc_r6_stale, _, err_r6_stale = run_cli(
            "sync-latest", "--batch", "20261127_verdict", "--repo-root", str(self.repo.root)
        )
        self.assertNotEqual(rc_r6_stale, 0)
        self.assertIn("frozen range", err_r6_stale)

        _write_reviewer_md(
            r6_path,
            base_sha=self.repo.deferred_sha,
            code_sha=vfix2_code,
            doc_sha=vfix2_doc,
            verdict="PASS",
            omit_code_head=True,
        )
        rc_r6_nocode, _, err_r6_nocode = run_cli(
            "sync-latest", "--batch", "20261127_verdict", "--repo-root", str(self.repo.root)
        )
        self.assertNotEqual(rc_r6_nocode, 0)
        self.assertIn("code_head", err_r6_nocode)

        # Valid 06 PASS: default sync-latest produces PASS (never CHANGES_REQUESTED!) & binds 06
        _write_reviewer_md(
            r6_path,
            base_sha=self.repo.deferred_sha,
            code_sha=vfix2_code,
            doc_sha=vfix2_doc,
            verdict="PASS",
        )
        rc_pass, out_pass, err_pass = run_cli(
            "sync-latest", "--batch", "20261127_verdict", "--repo-root", str(self.repo.root)
        )
        self.assertEqual(rc_pass, 0, err_pass)
        pass_meta = json.loads(out_pass)
        self.assertEqual(pass_meta["status"], "PASS")
        self.assertEqual(pass_meta["latest_codex"], "20261127_verdict/06_codex_recheck_r2.md")
        self.assertIn("确认 `PASS` 收口", latest_path.read_text(encoding="utf-8"))

        rc_pass_chk, out_pass_chk, _ = run_cli(
            "check",
            "--batch",
            "20261127_verdict",
            "--check-latest",
            "--strict-meta",
            "--json",
            "--repo-root",
            str(self.repo.root),
        )
        self.assertEqual(rc_pass_chk, 0, out_pass_chk)

        # Conflicting --status on closed PASS batch must FAIL
        rc_conf_p, _, err_conf_p = run_cli(
            "sync-latest",
            "--batch",
            "20261127_verdict",
            "--status",
            "CHANGES_REQUESTED",
            "--repo-root",
            str(self.repo.root),
        )
        self.assertNotEqual(rc_conf_p, 0)
        self.assertIn("conflicts with verified batch state", err_conf_p)

        # Valid 06 PASS_WITH_P2_P3 derives PASS_WITH_P2_P3 cleanly
        _write_reviewer_md(
            r6_path,
            base_sha=self.repo.deferred_sha,
            code_sha=vfix2_code,
            doc_sha=vfix2_doc,
            verdict="PASS_WITH_P2_P3",
        )
        rc_p23, out_p23, err_p23 = run_cli(
            "sync-latest", "--batch", "20261127_verdict", "--repo-root", str(self.repo.root)
        )
        self.assertEqual(rc_p23, 0, err_p23)
        self.assertEqual(json.loads(out_p23)["status"], "PASS_WITH_P2_P3")

        rc_p23_chk, out_p23_chk, _ = run_cli(
            "check",
            "--batch",
            "20261127_verdict",
            "--check-latest",
            "--strict-meta",
            "--json",
            "--repo-root",
            str(self.repo.root),
        )
        self.assertEqual(rc_p23_chk, 0, out_p23_chk)

    def test_status_empty_and_active(self) -> None:
        rc0, out0, _ = run_cli("status", "--json", "--repo-root", str(self.repo.root))
        self.assertEqual(rc0, 0)
        st0 = json.loads(out0)
        self.assertFalse(st0["latest_exists"])
        self.assertEqual(st0["batches"], [])
        self.assertEqual(st0["branch"], "feature/test")
        self.assertFalse(st0["dirty"])

        rc1, _, err1 = run_cli(
            "init",
            "--batch",
            "20261127_status",
            "--base",
            self.repo.deferred_sha,
            "--head",
            self.repo.doc_sha,
            "--repo-root",
            str(self.repo.root),
        )
        self.assertEqual(rc1, 0, err1)
        rc2, out2, _ = run_cli("status", "--json", "--repo-root", str(self.repo.root))
        self.assertEqual(rc2, 0)
        st1 = json.loads(out2)
        self.assertTrue(st1["latest_exists"])
        self.assertEqual(len(st1["batches"]), 1)
        self.assertEqual(st1["batches"][0]["batch"], "20261127_status")


if __name__ == "__main__":
    unittest.main()
