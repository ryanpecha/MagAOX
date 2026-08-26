# Backup of the ryanpecha/merge1 staged and unstaged work

Created 2026-08-26 10:11 UTC. Two local branches hold the two states as separate
commits, so the staged and unstaged work can be restored independently. Creating
them did not touch the index or the working tree.

| Branch | Commit | Contents |
| --- | --- | --- |
| `backup/merge1-staged` | `52a4343b` | The index exactly as staged: 176 files on top of HEAD `66a6087f`. |
| `backup/merge1-worktree` | `e7415953` | The unstaged working tree edits as a second commit on top of the staged commit: 166 files. |

The three untracked docs at the repo root (`CHANGES.md`, `DEV.md`,
`xwctest-macro-coverage-strategy.md`) are not in either commit. `DEV.md` contains a
password and must not be committed.

## What each diff means

```bash
git diff HEAD backup/merge1-staged                  # the staged work only
git diff backup/merge1-staged backup/merge1-worktree  # the unstaged work only
```

## How the branches were made

Git plumbing only. `write-tree` snapshots the real index. A throwaway copy of the
index is updated with `git add -u`, which picks up working tree content for tracked
files and ignores untracked files, and is then snapshotted the same way.

```bash
staged_tree=$(git write-tree)
c_staged=$(git commit-tree "$staged_tree" -p HEAD -m "backup: staged changes")

tmpidx=$(mktemp); cp .git/index "$tmpidx"
GIT_INDEX_FILE="$tmpidx" git add -u
wt_tree=$(GIT_INDEX_FILE="$tmpidx" git write-tree)
c_wt=$(git commit-tree "$wt_tree" -p "$c_staged" -m "backup: unstaged working tree changes")
rm -f "$tmpidx"

git branch backup/merge1-staged "$c_staged"
git branch backup/merge1-worktree "$c_wt"
```

## How to restore

Run from the repo root. The two commands are independent.

```bash
# Put the index back to the staged state. Touches only the index.
git read-tree backup/merge1-staged

# Put the working tree files back to the unstaged state. Touches only files on disk.
git restore --source=backup/merge1-worktree --worktree .
```

Running only the first command restores the staged-only state and drops the
unstaged comment pass.

## Making the backup survive the container

Local branches live on the `magaox-opt` volume with everything else. To keep a copy
elsewhere, push both branches to a fork or another remote you can write to:

```bash
git push <your-fork> backup/merge1-staged backup/merge1-worktree
```

Or write a single file that can be copied anywhere and fetched from later:

```bash
git bundle create merge1-backup.bundle backup/merge1-staged backup/merge1-worktree
```

## Cleanup

When the backups are no longer needed:

```bash
git branch -D backup/merge1-staged backup/merge1-worktree
```

## Note

An earlier index snapshot from this session, tree `59d3c28e`, is stale. The user
restaged files after it was taken. `backup/merge1-staged` supersedes it.
