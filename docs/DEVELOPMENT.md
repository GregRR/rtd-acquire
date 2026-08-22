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

## Portable C contract checks

Until a full C build system is justified, compile and run the portable C
contract tests directly with a C11 compiler:

```sh
cc -std=c11 -Wall -Wextra -Werror -pedantic \
  -I c/include \
  c/tests/test_spi_contract.c \
  -o /tmp/rtd-acquire-c-contract-tests
/tmp/rtd-acquire-c-contract-tests
```

The C gate will expand as the portable implementation grows. It must remain
usable with ordinary C11 toolchains and must not require Arduino-specific
headers for core tests.

## Cross-language conformance

Shared vectors under `conformance/` are the language-neutral behavioral
contract. Python vectors are required now. Once the C implementation can
consume a vector family, passing the same vectors in both languages becomes a
required gate for that shared feature. Cross-language conformance is therefore
activated feature-by-feature rather than blocked on C code that does not yet
exist.

Hardware-in-the-loop validation is a release/milestone gate, not a requirement
for every source commit.
