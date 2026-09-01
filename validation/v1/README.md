# Physical-validation record format v1

This directory contains the tracked template and capture-helper contract used to
record physical `rtd-acquire` validation without pasting ad hoc terminal output
into project documentation.

Actual bench records belong under `.rtd-acquire-local/validation/` while they
contain local hardware identifiers, raw captures, serial logs, or other
machine-specific material. Only reviewed conclusions and deliberately selected
reproducibility details should move into tracked project documentation.

The v1 workflow is:

1. create a local record directory with `validation.create_record`;
2. fill in the predeclared acceptance budget and hardware/source details in
   `record.md` **before** examining final measurements;
3. use `validation.capture_max31865` for structured Linux/Python MAX31865
   captures when applicable;
4. retain raw JSONL, summary JSON, capture manifests, calculations, and any C or
   serial evidence in the same local record directory; and
5. summarize only the claim depth actually supported after review.

The capture helper records public `Measurement` fields, normalized diagnostics,
and native evidence. It does not interpret RTD temperature models, decide an
acceptance budget, or automatically promote compatibility claims.

## Source-state provenance

`environment.json` records the current Git commit and `git_worktree_clean` as
metadata only; no dirty-worktree filenames are stored. Claim-bearing validation
should have `git_worktree_clean` equal to `true`. A `false` value documents that
the measurements came from a modified checkout and therefore cannot be
attributed to the recorded commit alone. `null` means Git state could not be
determined.
