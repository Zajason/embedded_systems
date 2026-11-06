"""Where results go: part2/results (mounted as /workspace/results in the container)."""
import csv
import os

RESULTS = os.environ.get("RESULTS_DIR",
                         os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "results"))
os.makedirs(RESULTS, exist_ok=True)


def write_csv(name, header, rows):
    path = os.path.join(RESULTS, name)
    with open(path, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(header)
        w.writerows(rows)
    print("wrote", path)
    return path
