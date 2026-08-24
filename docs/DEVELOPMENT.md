# Development quality gates

Run the applicable quality gates before each commit.

## Python and repository checks

```sh
uv run pytest
uv run ruff check src tests
uv run mypy --strict src tests
git diff --check
```

`pytest` includes the Python-side conformance-vector checks.

## Citation and provenance

`docs/REFERENCES.md` is the canonical bibliography for external technical
sources used by the project. When a change materially relies on a standard,
manufacturer data sheet/manual, industry document, application note, research
paper, validation dataset, or other external engineering source, add or verify
that source in `docs/REFERENCES.md` in the same change.

Keep short source comments at implementation or test points when they materially
improve traceability, but keep the full citation and project-role description
in `docs/REFERENCES.md`. Research sources retained for future hardware or
diagnostic work must be labeled as such and do not become an implementation
basis merely by being listed.

## Portable C contract checks

Until a full C build system is justified, compile and run the portable C
contract tests directly with a C11 compiler:

```sh
cc -std=c11 -Wall -Wextra -Werror -pedantic \
  -I c/include \
  c/tests/test_spi_contract.c \
  -o /tmp/rtd-acquire-c-spi-contract-test
/tmp/rtd-acquire-c-spi-contract-test

cc -std=c11 -Wall -Wextra -Werror -pedantic \
  -I c/include \
  c/tests/test_delay_contract.c \
  -o /tmp/rtd-acquire-c-delay-contract-test
/tmp/rtd-acquire-c-delay-contract-test

cc -std=c11 -Wall -Wextra -Werror -pedantic \
  -I c/include \
  c/src/core.c c/tests/test_core_contract.c \
  -o /tmp/rtd-acquire-c-core-contract-test
/tmp/rtd-acquire-c-core-contract-test

cc -std=c11 -Wall -Wextra -Werror -pedantic \
  -I c/include \
  c/src/core.c c/src/max31865.c c/tests/test_max31865_config.c \
  -o /tmp/rtd-acquire-c-max31865-config-test
/tmp/rtd-acquire-c-max31865-config-test

cc -std=c11 -Wall -Wextra -Werror -pedantic \
  -I c/include \
  c/src/core.c c/src/max31865.c c/tests/test_max31865_decode.c \
  -o /tmp/rtd-acquire-c-max31865-decode-test
/tmp/rtd-acquire-c-max31865-decode-test
```

The C gate will expand as the portable implementation grows. It must remain
usable with ordinary C11 toolchains and must not require Arduino-specific
headers for core tests.

Public C headers should carry concise comments for ownership, preconditions,
and failure/result semantics so the contract remains discoverable from an
editor. The `learn/api/c/` pages remain the fuller API reference and should be
updated in the same feature slice.

## Cross-language conformance

Shared vectors under `conformance/` are the language-neutral behavioral
contract. Python vectors are required now. Once the C implementation can
consume a vector family, passing the same vectors in both languages becomes a
required gate for that shared feature. Cross-language conformance is therefore
activated feature-by-feature rather than blocked on C code that does not yet
exist. Both current MAX31865 vector families now execute against Python and C;
the C runners are compiled by pytest when a host `cc` compiler is available,
while the strict standalone C contract gates remain mandatory. The current
measurement seed values are exactly representable in the C `float` path; the
general binary64/binary32 acceptance profile remains separate 0.2 work.

Hardware-in-the-loop validation is a release/milestone gate, not a requirement
for every source commit.

## Continuous integration

`.github/workflows/ci.yml` runs the Python test suite on every supported Python
minor version, currently 3.11 through 3.14. The Python 3.14 job also runs Ruff,
mypy strict, the portable C11 contract tests, tracked-file whitespace
validation, and a clean-tree check.

The release candidate should pass the same local gates before a tag is created;
CI is an independent confirmation, not a substitute for local artifact testing.

## Release automation

`.github/workflows/release.yml` validates a release tag, rebuilds the wheel and
source distribution with `uv build --no-sources`, inspects their contents, and
installs both artifacts into clean Python 3.11 and 3.14 environments for smoke
testing. It also verifies that the Raspberry Pi optional extra resolves and can
import its `spidev` dependency.

A manual `workflow_dispatch` run against a commit SHA, branch, or tag is
deliberately build-only. It can be used as a safe pre-tag release-workflow dry
run and does not publish to PyPI or modify a GitHub release.

Publishing occurs only for GitHub's `release: published` event. That event is
used for stable releases and prereleases. After validation succeeds, the
workflow attaches the wheel and source distribution to the existing GitHub
release and publishes the same artifacts to PyPI through Trusted Publishing.

Before the first PyPI release, configure the PyPI pending Trusted Publisher for:

- owner: `GregRR`;
- repository: `rtd-acquire`;
- workflow: `release.yml`;
- environment: `pypi`.

The `pypi` GitHub environment should use appropriate protection rules. No PyPI
API token is required when Trusted Publishing is configured correctly.
