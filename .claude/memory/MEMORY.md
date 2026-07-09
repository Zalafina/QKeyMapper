# QKeyMapper Project Memory

- [Qt Absolute-Positioning Window Resize Pattern](qt-no-layout-resize-pattern.md) — Core pattern for adding resize to dialogs without layout managers
- [Window Size Persistence](window-size-persistence.md) — INI-based save/restore with conditional checkbox
- [settingTabWidget Internal Resize](settingtabwidget-internal-resize.md) — Handling QTabWidget width changes with absolute-positioned pages
- [Qt 6 QTabBar setTabVisible Scroll Button Bug](qt-tabbar-settabvisible-bug.md) — Broken scroll button calculation and removeTab/insertTab workaround
- [QVariantMap/QVariantList Save/Load Pattern](qvariantmap-saveload-pattern.md) — Persisting struct data to QSettings using Qt-native serialization
- [Nested Brace Pitfalls in Save/Load Code](nested-brace-pitfalls.md) — Common bugs when modifying deeply nested serialization functions
- [Floating Button Sync Group Session](floating-button-sync-group-session.md) — Full session record: 6 files, 5 bugs, design decisions, and lessons from implementing sync group feature
- [Qt QSpinBox Incremental Delta Pattern](qt-incremental-delta-spinbox.md) — valueChanged fires per-step; use incremental delta not cumulative from backup
- [Bidirectional Cross-Tab Sync](bidirectional-cross-tab-sync.md) — Sync checks must handle both A→B and B→A directions using display context
- [.ui Button → Menu + State Member Migration](ui-button-to-menu-migration.md) — Step-by-step pattern for replacing .ui button dependencies with menu actions + member state
- [Category Filter Header Color Indicator](category-filter-header-color.md) — Visual filter state via QHeaderView foreground color, replacing removed button text summary
