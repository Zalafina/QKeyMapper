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

sys.path.insert(0, str(Path(__file__).resolve().parent))
import review_helper  # noqa: E402


def _remove_readonly(func, path, exc_info):
    """Clear read-only attributes on Windows .git object files during cleanup."""
    if os.path.islink(path) or (getattr(os.lstat(path), "st_file_attributes", 0) & review_helper.FILE_ATTRIBUTE_REPARSE_POINT):
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
    """Self-cleaning temporary Git repository with multiple commits and simulated upstream."""

    def __init__(self) -> None:
        self._tmp = tempfile.mkdtemp(prefix="qkm_review_test_")
        self.root = Path(self._tmp)
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
        self._git("commit", "-m", "chore: initial baseline")
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
        if os.path.exists(self._tmp):
            shutil.rmtree(self._tmp, onerror=_remove_readonly)


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

        # Exclusive creation always refuses duplicate init (and --force option no longer exists)
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

    def test_write_guards_junction_and_hardlink(self) -> None:
        # 1. Junction at out/codex_review -> src/ must be rejected before creating anything
        target_dir = self.repo.root / "src"
        out_dir = self.repo.root / "out"
        out_dir.mkdir(exist_ok=True)
        review_dir = out_dir / "codex_review"
        _create_dir_junction(review_dir, target_dir)
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
        self.assertFalse((target_dir / "20261127_junc").exists())

        # status & check must also refuse junction under out/codex_review
        rc_st, _, err_st = run_cli("status", "--repo-root", str(self.repo.root))
        self.assertNotEqual(rc_st, 0)
        self.assertIn("reparse", err_st.lower())
        os.rmdir(review_dir)

        # 2. Hardlinked LATEST.md must refuse before writing 01_gemini_submission.md (no partial init)
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
        self.assertEqual(tracked_canary.read_text(encoding="utf-8"), "# Notes\nline2\n")

    def test_guards_dirty_tree_bad_batch_range_and_pushed(self) -> None:
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

        # Reject already-pushed range (upstream == head)
        rc_pushed, _, err_pushed = run_cli(
            "init",
            "--batch",
            "20261127_pushed",
            "--base",
            f"{self.repo.upstream_sha}~0",
            "--head",
            self.repo.upstream_sha,
            "--repo-root",
            str(self.repo.root),
        )
        self.assertNotEqual(rc_pushed, 0)
        self.assertTrue("empty" in err_pushed.lower() or "pushed" in err_pushed.lower())

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

        # Reject detached / wrong branch head not on current branch
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

    def test_handoff_profile_trigger_check(self) -> None:
        # Changing top-level handoff/README.md alone keeps general profile valid
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

        # Changing handoff/<task>/plan.md with --profile general must fail
        task_dir = handoff_dir / "mcp_task"
        task_dir.mkdir()
        (task_dir / "plan.md").write_text("# Plan\n", encoding="utf-8", newline="\n")
        self.repo._git("add", "handoff/mcp_task/plan.md")
        self.repo._git("commit", "-m", "docs(mcp): update task plan")
        plan_sha = self.repo._rev_parse("HEAD")
        rc_gen, _, err_gen = run_cli(
            "init",
            "--batch",
            "20261127_handoff",
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
            "20261127_handoff",
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

    def test_check_read_boundaries_links_round_sequence_and_strict_meta(self) -> None:
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

        batch_dir = self.repo.root / "out" / "codex_review" / "20261127_links"
        # Create an outside file & a junction inside docs pointing out (or inside repo) to test link traversal block
        outside_file = Path(self.repo._tmp).parent / "qkm_outside_secret.txt"
        outside_file.write_text("secret\n", encoding="utf-8", newline="\n")
        self.addCleanup(lambda: outside_file.unlink(missing_ok=True))

        junc_inside = self.repo.root / "docs" / "junc_src"
        _create_dir_junction(junc_inside, self.repo.root / "src")

        r2_path = batch_dir / "02_codex_review_r1.md"
        r2_path.write_text(
            "# Review\n"
            "[ok](../../../src/main.cpp#L1-L3)\n"
            "[web](https://example.com/foo)\n"
            "Inline `[ignored](../../../does/not/exist.txt)`\n"
            "```markdown\n[ignored_fence](../../../nope.md)\n```\n"
            "[empty](   )\n"
            "[abs](C:/Windows/win.ini)\n"
            "[unc](\\\\server\\share\\x.txt)\n"
            "[escape](../../../../../qkm_outside_secret.txt)\n"
            "[junc_link](../../../docs/junc_src/main.cpp)\n"
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
        parsed = json.loads(out_bad)
        joined = "\n".join(parsed["errors"])
        self.assertIn("empty link target", joined)
        self.assertIn("C:/Windows/win.ini", joined)
        self.assertIn("\\\\server\\share\\x.txt", joined)
        self.assertIn("escapes repository root", joined)
        self.assertIn("reparse", joined.lower())
        self.assertIn("missing_file.cpp", joined)
        self.assertIn("L1-L999", joined)
        self.assertNotIn("does/not/exist.txt", joined)
        self.assertNotIn("nope.md", joined)

        # Clean valid 02_codex_review_r1.md
        r2_path.write_text("# Review\n[ok](../../../src/main.cpp#L1-L3)\n", encoding="utf-8", newline="\n")

        # Invalid round filenames (wrong kind / suffix / duplicate / gap)
        bad_r2_recheck = batch_dir / "02_codex_recheck_r1.md"
        bad_r2_recheck.write_text("# wrong kind\n", encoding="utf-8", newline="\n")
        bad_r3 = batch_dir / "03_gemini_submission.md"
        bad_r3.write_text("# wrong 03 kind\n", encoding="utf-8", newline="\n")
        rc_seq, out_seq, _ = run_cli(
            "check",
            "--batch",
            "20261127_links",
            "--json",
            "--repo-root",
            str(self.repo.root),
        )
        self.assertEqual(rc_seq, 1)
        joined_seq = "\n".join(json.loads(out_seq)["errors"])
        self.assertIn("duplicate sequence index 02", joined_seq)
        self.assertIn("non-canonical filename", joined_seq)
        bad_r2_recheck.unlink()
        bad_r3.unlink()

        # Test strict-meta catches drifting HEAD, wrong post_review_action, missing code_head, and LATEST mismatch
        sub = batch_dir / "01_gemini_submission.md"
        orig_sub = sub.read_text(encoding="utf-8")
        bad_sub = orig_sub.replace(
            f"`{self.repo.deferred_sha[:8]}..{self.repo.doc_sha[:8]}`",
            f"`{self.repo.deferred_sha[:8]}..HEAD`",
        ).replace("`pause_for_user`", "`invalid_action`")
        sub.write_text(bad_sub, encoding="utf-8", newline="\n")
        rc_meta, out_meta, _ = run_cli(
            "check",
            "--batch",
            "20261127_links",
            "--strict-meta",
            "--json",
            "--repo-root",
            str(self.repo.root),
        )
        self.assertEqual(rc_meta, 1)
        joined_meta = "\n".join(json.loads(out_meta)["errors"])
        self.assertIn("invalid post_review_action", joined_meta)
        self.assertIn("drifting refs like HEAD are forbidden", joined_meta)

        # Restore submission, break LATEST.md so batch only appears in historical note
        sub.write_text(orig_sub, encoding="utf-8", newline="\n")
        latest_path = self.repo.root / "out" / "codex_review" / "LATEST.md"
        orig_latest = latest_path.read_text(encoding="utf-8")
        latest_path.write_text(
            orig_latest.replace("`20261127_links`（", "`99999999_other`（").replace(
                "- **前序已闭环批次**", "- **前序已闭环批次**：20261127_links\n- **备注**"
            ),
            encoding="utf-8",
            newline="\n",
        )
        rc_lat, out_lat, _ = run_cli(
            "check",
            "--batch",
            "20261127_links",
            "--check-latest",
            "--strict-meta",
            "--json",
            "--repo-root",
            str(self.repo.root),
        )
        self.assertEqual(rc_lat, 1)
        self.assertIn("active batch field must start with `20261127_links`", "\n".join(json.loads(out_lat)["errors"]))

        # Missing canonical 01_gemini_submission.md must fail --strict-meta
        latest_path.write_text(orig_latest, encoding="utf-8", newline="\n")
        sub.unlink()
        rc_nosub, out_nosub, _ = run_cli(
            "check",
            "--batch",
            "20261127_links",
            "--strict-meta",
            "--json",
            "--repo-root",
            str(self.repo.root),
        )
        self.assertEqual(rc_nosub, 1)
        self.assertIn("missing canonical '01_gemini_submission.md'", "\n".join(json.loads(out_nosub)["errors"]))

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
