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
    os.chmod(path, stat.S_IWRITE)
    func(path)


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
        (self.root / "src" / "main.cpp").write_text("int main() {\n    return 0;\n}\n", encoding="utf-8", newline="\n")
        self._git("add", ".")
        self._git("commit", "-m", "chore: initial baseline")
        self.upstream_sha = self._rev_parse("HEAD")
        # Simulate local remote-tracking ref pointing at upstream_sha
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
            ["git", *args],
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
            "handoff-plan",
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

        # Strict check must pass on generated files
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

        # Refuse accidental overwrite without --force
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

    def test_guards_dirty_tree_bad_batch_and_bad_range(self) -> None:
        # Reject path traversal batch names
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

    def test_check_detects_broken_links_line_bounds_bom_crlf_and_sequence(self) -> None:
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
        # Add 02 review with valid link, fenced/inline ignored links, then invalid link + out-of-range #L
        r2_path = batch_dir / "02_codex_review_r1.md"
        r2_path.write_text(
            "# Review\n"
            "[ok](../../../src/main.cpp#L1-L3)\n"
            "Inline `[ignored](../../../does/not/exist.txt)`\n"
            "```markdown\n[ignored_fence](../../../nope.md)\n```\n"
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
        self.assertIn("missing_file.cpp", joined)
        self.assertIn("L1-L999", joined)
        self.assertNotIn("does/not/exist.txt", joined)
        self.assertNotIn("nope.md", joined)

        # Fix links, then add CRLF and gap sequence 04_codex_recheck_r1.md without 03
        r2_path.write_text("# Review\n[ok](../../../src/main.cpp#L1-L3)\n", encoding="utf-8", newline="\n")
        bad_seq = batch_dir / "04_codex_recheck_r1.md"
        bad_seq.write_bytes(b"\xef\xbb\xbf# Recheck\r\n")
        rc_fmt, out_fmt, _ = run_cli(
            "check",
            "--batch",
            "20261127_links",
            "--json",
            "--repo-root",
            str(self.repo.root),
        )
        self.assertEqual(rc_fmt, 1)
        joined2 = "\n".join(json.loads(out_fmt)["errors"])
        self.assertIn("UTF-8 BOM", joined2)
        self.assertIn("CRLF", joined2)
        self.assertIn("expected sequence 03", joined2)

        # Strict-meta catches missing post_review_action
        bad_seq.unlink()
        sub = batch_dir / "01_gemini_submission.md"
        sub.write_text("# Minimal submission\n", encoding="utf-8", newline="\n")
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
        self.assertIn("post_review_action", "\n".join(json.loads(out_meta)["errors"]))

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
