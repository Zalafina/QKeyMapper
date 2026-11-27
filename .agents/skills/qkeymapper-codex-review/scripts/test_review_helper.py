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

        r2_path.write_text("# Review\n[ok](../../../src/main.cpp#L1-L3)\n", encoding="utf-8", newline="\n")

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
