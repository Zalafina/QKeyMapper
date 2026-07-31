# Large Source Rename Safety

For the large `QKeyMapper/qkeymapper.cpp`, repeated `apply_patch` calls can report success while truncating the file. Before editing, compare line count and working-tree diff size. For broad mechanical renames, generate a unified diff from the `HEAD` blob, run `git apply --check --ignore-space-change --whitespace=error-all`, then apply it once. Keep a scoped recovery path using a temporary Git index when the repository index is not writable.
