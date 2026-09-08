# CI, Builds, and Packaging

## Initial CI jobs

1. Repository validation
2. C++ compile
3. Asset naming check
4. Automated smoke map launch where infrastructure permits
5. Packaged Development build
6. Archive build logs

## Build types

- Editor development
- Development packaged build
- Test build
- Shipping build later

## Build metadata

Every packaged build should expose:

```text
version
commit_hash
build_time
engine_version
content_revision
```

## Release checklist

- Clean checkout build succeeds
- No missing assets
- No unexpected redirectors
- Save compatibility tested
- Settings reset tested
- Controller disconnected/reconnected tested
- Benchmark route captured
- Credits and third-party notices updated
