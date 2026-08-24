# Native evidence

**Introduced in:** `rtd-acquire 0.1.0a1`

A normalized diagnostic answers **what common condition can the application
rely on?** Native evidence answers **what did this device or protocol actually
report?**

`NativeEvidence` can carry an original identifier, an original message, or
both:

```python
from rtd_acquire import NativeEvidence

evidence = NativeEvidence(
    identifier="D7",
    message="RTDIN- > 0.85 x VBIAS",
)
```

A `Diagnostic` may contain multiple native-evidence records when more than one
native observation supports the same normalized condition.

## Why preserve it?

Native evidence is valuable for:

- troubleshooting hardware;
- comparing behavior across device revisions;
- logging and audit trails;
- validating normalization mappings; and
- avoiding invented combined vendor identifiers.

Applications can use the normalized code for portable control flow while still
retaining the original evidence for engineering detail.
