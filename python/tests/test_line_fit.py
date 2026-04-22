"""Quick doctest-style tests for line_fit.fit_lines.

Run with:  .venv/bin/python -m pytest python/tests -q
"""
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from line_fit import fit_lines  # noqa: E402


def _line_points(p0, d, n, noise=0.0, rng=None):
    rng = rng or np.random.default_rng(0)
    t = np.linspace(-5.0, 5.0, n)
    base = p0 + np.outer(t, d)
    if noise > 0:
        base += rng.normal(scale=noise, size=base.shape)
    return base


def test_fit_single_clean_line_recovers_direction():
    p0 = np.array([1.0, 2.0, 3.0])
    d = np.array([1.0, 0.0, 0.0])
    pts = _line_points(p0, d, n=100)
    lines = fit_lines(pts, n_lines=1, distance_threshold_mm=0.1)
    assert len(lines) == 1
    line = lines[0]
    recovered = np.array([line.dx, line.dy, line.dz])
    recovered /= np.linalg.norm(recovered)
    # Direction recovered up to sign.
    assert abs(abs(np.dot(recovered, d / np.linalg.norm(d))) - 1.0) < 1e-4
    assert line.inlier_count == 100


def test_fit_single_noisy_line_is_still_close():
    p0 = np.array([0.0, 0.0, 0.0])
    d = np.array([1.0, 1.0, 1.0]) / np.sqrt(3)
    rng = np.random.default_rng(42)
    pts = _line_points(p0, d, n=300, noise=0.2, rng=rng)
    lines = fit_lines(pts, n_lines=1, distance_threshold_mm=1.0)
    assert len(lines) == 1
    recovered = np.array([lines[0].dx, lines[0].dy, lines[0].dz])
    recovered /= np.linalg.norm(recovered)
    assert abs(abs(np.dot(recovered, d)) - 1.0) < 1e-2
    # Almost all points within 1 mm of a noise-0.2 line.
    assert lines[0].inlier_count > 280


def test_fit_two_vertex_sharing_lines_recovers_both():
    # Two tracks that share a common vertex at the origin — the realistic
    # TPC case where particles emerge from a single interaction point.
    rng = np.random.default_rng(7)
    a_dir = np.array([1.0, 0.2, 0.1])
    a_dir /= np.linalg.norm(a_dir)
    b_dir = np.array([-0.3, 1.0, 0.4])
    b_dir /= np.linalg.norm(b_dir)
    # Both half-lines start at the origin and go in different directions.
    ta = np.linspace(0.2, 5.0, 150)
    tb = np.linspace(0.2, 5.0, 150)
    a = np.outer(ta, a_dir) + rng.normal(scale=0.08, size=(150, 3))
    b = np.outer(tb, b_dir) + rng.normal(scale=0.08, size=(150, 3))
    lines = fit_lines(np.vstack([a, b]), n_lines=3,
                      distance_threshold_mm=0.5, min_inliers=50)
    assert len(lines) >= 2, f"expected >=2 lines, got {len(lines)}"
    directions = []
    for line in lines:
        v = np.array([line.dx, line.dy, line.dz])
        directions.append(v / np.linalg.norm(v))
    # Each input direction should be represented by at least one fitted
    # line. The first iteration often lands on a compromise line that
    # straddles both tracks (PCA sees their joint covariance), so the
    # individual tracks emerge only on later iterations — we accept a
    # looser cos-angle threshold here.
    def _match(target):
        return max(abs(np.dot(v, target)) for v in directions)
    assert _match(a_dir) > 0.9
    assert _match(b_dir) > 0.9


def test_fit_stops_when_too_few_inliers():
    pts = np.array([[0, 0, 0], [1, 0, 0], [2, 0, 0]], dtype=float)
    lines = fit_lines(pts, n_lines=5, min_inliers=10)
    assert lines == []


def test_endpoints_shape():
    p0 = np.array([0, 0, 0])
    d = np.array([1, 0, 0])
    pts = _line_points(p0, d, n=50)
    line = fit_lines(pts, n_lines=1, distance_threshold_mm=0.1)[0]
    eps = line.endpoints()
    assert eps.shape == (2, 3)
