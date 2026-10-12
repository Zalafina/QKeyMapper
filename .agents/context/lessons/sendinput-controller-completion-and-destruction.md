# SendInputTask controller completion and destruction

Verified on 2026-10-12 against Qt 6.8.3 and Qt 5.15.2. Recheck the current implementation before applying these constraints.

- QThreadPool::clear() removes queued runnables; it does not finish active tasks. Joining the owning worker QThread does not join its independent pools. The queued all-keys-released signal may precede a runnable's last controller access.
- A condition wake is not completion. Stop/cancel and the wait predicate must use the same mutex, then wait for every accepted task before resetting or deleting its mutex, condition and flags. Keep queued KEY_UP releases while cancelling queued KEY_DOWN work.
- Protect a borrowed controller pointer for its full use. Release the map mutex before waiting for tasks that can borrow through that mutex, while keeping the owner lane from mutating the map during the wait. Do not clear any entry while another task can access it.
- Drain before GUI/mapping-data destruction. A later static worker destructor must not re-enter UI-dependent stop logic that was already completed.
- Log runnable fields before QThreadPool::start() when autoDelete is enabled; a fast task can finish and delete itself before post-start logging.
- Validate an active timed wait, active pause, queued releases, cancel/recreate, pause/continue, stop/restart, late admission, concurrent borrowers and actual product-main exit. Use the maintained sendinput_lifecycle_test.pro target with actual ASan instrumentation and configuration-specific static analysis.
- Unknown no-output test keys exercise real SendInputTask lifetime and KEY_UP execution, not physical keyboard/Hook/driver release. A genuine completion wait can still be delayed by an existing RunWait or blocked driver operation; a timeout cannot justify deleting live synchronization resources.

The old cleanup order produced real-task ASan reads after free in timed-wait and pause paths. Investigation evidence and coverage boundaries are archived separately in plan/ui_scale_handoff/validation_reports/17_threadpool_lifecycle_validation.md; ignored plan/out data must be migrated separately.
