# RFC-001: Storage Read/Write API

**Status:** Draft — under review

## 1. Problem

PULSE is being built by working on two different parts of the
same engine: the storage engine (durability, on-disk format, indexing,
compaction) and the query engine (parsing PULSE-QL, planning, executing
queries).

The right approach is to define, in writing, and agree on the exact contract between the contributors: what functions exist, what they take as input, what they return, and what they promise to
do. This contract is called the storage API in this document. Once it's
agreed, the storage engine (Track A) can build the real, durable
implementation behind it, while the query engine (Track B) builds against a
trivial fake implementation of the same contract.

This document proposes that contract.

## 2. Background: what data are we actually storing?

Before the API makes sense, it helps to be explicit about what a single
"data point" represents. In this project, a data point is one telemetry
reading : e.g., "GPU temperature on node 3 was 68.2°C at this exact
nanosecond, as part of job 4821." Every single reading a cluster produces
becomes one of these.

There will be a huge number of these; potentially thousands per second
across a whole cluster, and once written, a point is never modified, only
eventually deleted once it's old enough to fall outside the retention
window. This "write-heavy, append-only, never-updated" shape is the single
most important fact about this workload, and it's what justifies every
performance-oriented decision later in the project (LSM-tree storage,
time-based partitioning, etc.) but for this RFC, it's specifically why the
API is shaped the way it is: it needs to make writing a single point as fast
and simple as possible.

## 3. Proposal

### 3.1 Core data type: `Point`

```cpp
struct Point {
    std::string metric;       // e.g. "gpu_temp", "cpu_util", "mem_used_bytes"
    std::string job_id;       // e.g. "job-4821" — the scheduler job this reading belongs to
    std::unordered_map<std::string, std::string> tags;  // e.g. {"node": "n03", "rack": "r2", "gpu_index": "0"}
    int64_t timestamp;        // unix epoch time, in nanoseconds
    double value;              // the actual reading, e.g. 68.2
};
```

**Walkthrough of a real example.** Suppose node `n03`, which is running job
`job-4821`, reports that GPU 0's temperature is 68.2°C at a specific instant.
That becomes:

```cpp
Point{
    .metric = "gpu_temp",
    .job_id = "job-4821",
    .tags = {{"node", "n03"}, {"rack", "r2"}, {"gpu_index", "0"}},
    .timestamp = 1755280000123456789,
    .value = 68.2
};
```

**Why `job_id` is its own field, not just another entry inside `tags`.**
This is the single most important design decision in this whole RFC, so it's
worth explaining carefully, not just asserting.

Every existing time-series database (InfluxDB, TimescaleDB) and every
existing HPC monitoring tool we researched (ClusterCockpit, DCDB, MetricQ)
treats "which job does this belong to" as just another generic tag, exactly
like "which rack" or "which node." That means the storage engine has no
special awareness that job matters more than, say, rack, it's just one key
among many in a generic map. As a result, if you want to ask "show me
everything from job 4821, ordered by how far into the job each reading was,"
you can't ask the database that directly; you have to fetch data filtered
by the job tag, then do extra work in your own application to figure out
when the job actually started and re-align every timestamp yourself.

By making `job_id` a real, separate, structural field on `Point` (not buried
in the generic `tags` map), we give the storage engine a way to know, at the
lowest level, that "which job" is a first-class concept, which means later
on, we can build a dedicated index just for job lookups, and
the query language can eventually offer job-relative queries natively
(e.g., "align this job's data to when it started"; a feature we're not
building yet, but are deliberately leaving room for). This single choice is
what makes PULSE meaningfully different from every existing tool, rather
than a smaller version of something that already exists — so every other
decision in this project should be checked against whether it protects this
property.

**Why `tags` stays a generic `unordered_map`, when `job_id` doesn't.**
Tags like "which rack," "which node," "which GPU index" are genuinely
open-ended — different clusters will want to track different dimensions,
and we can't (and shouldn't) hard-code every possible one into the struct.
A generic key-value map is the right tool for genuinely arbitrary,
user-defined dimensions. `job_id` isn't arbitrary in the same way; it's a
concept the whole system is designed around — which is why it gets
special-cased and everything else doesn't.

**Why timestamps are nanoseconds, not seconds or milliseconds.**
Cluster telemetry can realistically be sampled many times per second
(especially GPU utilization, which monitoring agents sometimes poll very
frequently). If timestamps were only second-resolution, two readings taken
within the same second would either silently overwrite each other or have
no reliable way to be ordered relative to one another. Nanosecond resolution
(still just a 64-bit integer, `int64_t` — no extra storage cost) removes
this problem entirely, at zero cost. This is a small decision, but it's the
kind of small decision that, done correctly up front, prevents a subtle bug
much later that would be painful to retrofit.

### 3.2 Write API

```cpp
struct Result {
    bool ok;
    std::string error_message;  // empty string if ok == true
};

Result write(const Point& point);
```

**Example usage (from the ingestion side, hypothetically):**
```cpp
Point p{ "gpu_temp", "job-4821", {{"node", "n03"}}, 1755280000123456789, 68.2 };
Result r = write(p);
if (!r.ok) {
    // handle failure — e.g. log it, retry, or surface to the caller
    std::cerr << "Write failed: " << r.error_message << std::endl;
}
```

**Why the function only takes a single `Point`, not a batch of many at
once, even though writes will happen very frequently.** Batching (writing
many points in a single call) is a real and valuable optimization; it
reduces the number of times we need to touch the write-ahead log, which
matters a lot for throughput. But batching also introduces a genuinely hard
question we don't need to answer yet: what happens if point 3 of a batch of
10 fails to write, but the other 9 succeed? Do we roll all of them back? Do
we return which ones failed? That's real complexity, and taking it on before
even the simple single-point path is built and proven correct would be
solving a problem we don't have yet. The plan is: get single-point writes
working and correct first, then — only once that's solid; add a
`write_batch(const std::vector<Point>& points)` overload purely as a
performance improvement, without changing or breaking this original
function at all.

**Why `Result` instead of throwing a C++ exception on failure.** Exceptions
in C++ carry real runtime cost (unwinding the stack, and depending on the
compiler and settings, sometimes disabling certain optimizations in
surrounding code), which matters on a function that might be called
thousands of times per second; the write path is about as "hot" (frequently
executed) as any part of this system gets. Beyond performance, a `Result`
return type is also more explicit: the caller (in this project's case,
mainly Track B, or an ingestion script) is forced to look at whether the
write succeeded, rather than a failure silently throwing an exception
somewhere that might not be caught at all. This is a standard, well-regarded
pattern in modern systems-level C++ for exactly these reasons.

### 3.3 Read API

```cpp
std::vector<Point> read(
    const std::string& metric,
    const std::optional<std::string>& job_id,
    const std::unordered_map<std::string, std::string>& tags_filter,
    int64_t start_ts,
    int64_t end_ts
);
```

**Example usage:** "give me all `gpu_temp` readings for job 4821, on rack
r2, from the last hour":
```cpp
auto results = read(
    "gpu_temp",
    "job-4821",
    {{"rack", "r2"}},
    now_ns() - 3600LL * 1'000'000'000LL,
    now_ns()
);
```

**Why the function returns a `std::vector<Point>` rather than something
like a streaming iterator or generator.** For very large result sets (e.g.,
a query spanning a very wide time range), a `std::vector` means every
matching point is loaded into memory before the function returns anything
at all, which is not the most memory-efficient design possible. A
streaming interface (where the caller gets points one at a time, or in
small chunks, without the whole result ever existing in memory at once)
would be the more scalable long-term choice. However, building that
correctly means dealing with real complexity around object lifetimes,
especially once concurrent reads, writes, and background compaction are all
happening at the same time. and that complexity isn't
worth taking on before the simpler, correct version exists and we have real
evidence (from benchmarking, Phase 8) that it's actually a bottleneck. This
is explicitly listed as an open question below, not a silent shortcut.

## 4. Open ended questions (meant for discussions later on)

1. **Streaming reads.** Do we accept the `std::vector`-returning read API
   for v1 and revisit only once real benchmarking shows it's
   actually a bottleneck, or is there a lightweight way to make this
   streaming from day one without much added complexity? <u>*Proposed answer:
   accept for v1*</u>
2. **Sort order guarantee.** Should `read()` promise that returned points
   come back already sorted by timestamp, or is sorting left as the query
   engine's responsibility? <u>*Proposed answer: storage guarantees
   timestamp-sorted output, since this will be nearly free once we're
   reading from time-ordered on-disk segments later, and it saves Track B
   from redundant re-sorting on every query.*</u>
3. **Nanosecond timestamps — actually necessary, or overkill?** Confirm
   this resolution genuinely matches what real telemetry sources (or our
   synthetic data generator) will produce. 
   <u>*Proposed answer: keep
   nanoseconds regardless, there's no cost to doing so even if actual
   sampling rates turn out to be coarser.*</u>
4. **What happens if `write()` is called with an empty `job_id`?** Is that
   a valid "this reading isn't tied to any job" case, or should it be
   rejected as invalid input? 
   <u>*Proposed answer: needs discussion,
   since it affects how strictly the job index needs to handle
   missing values.*</u>