"""End-to-end check of the mapf_solver command line.

Runs the executable the way the benchmark runner does and verifies the
contract the runner relies on: exit codes, and a result file that is valid
JSON with the documented keys.

Usage: check-solver-cli.py <mapf_solver executable> <tests/data directory>
"""

import json
import subprocess
import sys
import tempfile
from pathlib import Path

EXPECTED_KEYS = [
    "schema_version", "map", "scen", "agents", "solver", "w", "conflict_policy",
    "seed", "timeout_s", "status", "error", "validity", "soc", "makespan",
    "lb_independent", "lb_frontier", "lb_certified", "lb_source",
    "frontier_consistent", "hl_expanded", "hl_generated", "ll_calls",
    "ll_expanded", "ll_generated", "runtime_s", "runtime_lowlevel_s",
    "runtime_detect_s", "peak_memory_bytes", "compiler", "build_type",
]
STATUSES = {"solved", "timeout", "no_solution", "error", "invalid"}

failures = []


def check(condition, message):
    if not condition:
        failures.append(message)


def run(solver, *args):
    return subprocess.run([solver, *args], capture_output=True, text=True, timeout=120)


def main():
    solver, data = sys.argv[1], Path(sys.argv[2])
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)

        # A normal run writes a complete result file and exits with 0.
        out = tmp / "tiny.json"
        done = run(solver, "--map", str(data / "tiny.map"), "--scen", str(data / "tiny.scen"),
                   "--agents", "3", "--solver", "cbs", "--timeout", "5", "--output", str(out))
        check(done.returncode == 0, f"normal run: exit code {done.returncode}, expected 0")
        check(out.exists(), "normal run: no result file")
        if out.exists():
            result = json.loads(out.read_text(encoding="utf-8"))
            check(list(result.keys()) == EXPECTED_KEYS, f"normal run: keys are {list(result.keys())}")
            check(result.get("schema_version") == 1, "normal run: schema_version is not 1")
            check(result.get("agents") == 3, "normal run: agents is not 3")
            check(result.get("status") in STATUSES, f"normal run: status {result.get('status')!r}")
            check(result.get("map") == str(data / "tiny.map"), "normal run: map path changed")
            # Shortest paths of the three agents in tiny.scen are 4, 5 and 3.
            check(result.get("lb_independent") == 12,
                  f"normal run: lb_independent is {result.get('lb_independent')}, expected 12")
            check(isinstance(result.get("peak_memory_bytes"), int) and result["peak_memory_bytes"] > 0,
                  "normal run: peak_memory_bytes is not a positive integer")
            if result.get("status") == "solved":
                check(isinstance(result.get("soc"), int) and result["soc"] >= 12,
                      "normal run: solved without a sum-of-costs of at least 12")

        # An agent that cannot reach its goal is reported as no_solution.
        out = tmp / "walled.json"
        done = run(solver, "--map", str(data / "walled.map"), "--scen", str(data / "walled.scen"),
                   "--agents", "1", "--solver", "cbs", "--output", str(out))
        check(done.returncode == 0, f"walled run: exit code {done.returncode}, expected 0")
        if out.exists():
            result = json.loads(out.read_text(encoding="utf-8"))
            check(result.get("status") == "no_solution", f"walled run: status {result.get('status')!r}")
            check("cannot reach its goal" in result.get("error", ""), "walled run: error text missing")
        else:
            check(False, "walled run: no result file")

        # An unreadable map still produces a result file, with exit code 1.
        out = tmp / "missing.json"
        done = run(solver, "--map", str(data / "no_such.map"), "--scen", str(data / "tiny.scen"),
                   "--agents", "1", "--solver", "cbs", "--output", str(out))
        check(done.returncode == 1, f"missing map: exit code {done.returncode}, expected 1")
        if out.exists():
            result = json.loads(out.read_text(encoding="utf-8"))
            check(result.get("status") == "error", f"missing map: status {result.get('status')!r}")
            check("cannot open map file" in result.get("error", ""), "missing map: error text missing")
        else:
            check(False, "missing map: no result file")

        # A command-line error exits with 2, explains itself and writes nothing.
        out = tmp / "usage.json"
        done = run(solver, "--map", "a.map", "--bogus", "1", "--output", str(out))
        check(done.returncode == 2, f"bad option: exit code {done.returncode}, expected 2")
        check("unknown option '--bogus'" in done.stderr, "bad option: message missing on stderr")
        check(not out.exists(), "bad option: a result file was written")

        # --help prints the usage text and exits with 0.
        done = run(solver, "--help")
        check(done.returncode == 0, f"help: exit code {done.returncode}, expected 0")
        check(done.stdout.startswith("Usage: mapf_solver"), "help: usage text missing on stdout")

        # An unwritable result path exits with 3.
        done = run(solver, "--map", str(data / "tiny.map"), "--scen", str(data / "tiny.scen"),
                   "--agents", "1", "--solver", "cbs", "--output", str(tmp / "no_such_dir" / "r.json"))
        check(done.returncode == 3, f"unwritable output: exit code {done.returncode}, expected 3")

    for message in failures:
        print("FAIL:", message)
    print(f"{len(failures)} failure(s)")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
