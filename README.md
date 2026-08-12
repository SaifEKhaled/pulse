# PULSE

**A purpose-built, embeddable time-series storage and query engine for HPC cluster telemetry, with job-native queries.**

> Status: early development. This README will grow as the engine takes shape.

## What this is

Most time-series databases (InfluxDB, TimescaleDB, Prometheus) and most existing HPC
monitoring stacks (ClusterCockpit, DCDB, MetricQ) are built for large, centrally-run
HPC computing centers with dedicated ops teams. PULSE targets the opposite end: a
single-binary, zero-external-dependency engine for small clusters, department labs,
or individual research groups — where a job (not a raw timestamp) is the natural
unit of query.

## Why it exists

See [`docs/design.md`](docs/design.md) (yet to be made) for the full problem statement, architecture,
and prior-art comparison (ClusterCockpit, DCDB, MetricQ, InfluxDB, TimescaleDB).

## Status

This project is under active joint development by two contributors:
- **Storage engine** (WAL, on-disk format, compaction, concurrency)
- **Query engine** (PULSE-QL parser, planner, parallel executor)

Nothing here is production-ready yet. Build instructions will be added once the
first buildable skeleton lands.

## License

See [`LICENSE`](LICENSE).