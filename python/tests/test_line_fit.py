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


def test_fit_dominant_track_in_mixed_cloud():
    # PCA-based fit_lines latches onto the dominant direction in a mixed
    # cloud. Here A has 5x the points and 2x the length of B, so the
    # first iteration should recover a_dir very closely. Multi-track
    # recovery beyond the dominant one is best-effort with iterative
    # PCA and is NOT guaranteed by this test.
    rng = np.random.default_rng(7)
    a_dir = np.array([1.0, 0.0, 0.0])
    b_dir = np.array([0.0, 1.0, 0.0])
    ta = np.linspace(0.0, 20.0, 500)
    tb = np.linspace(0.0, 5.0, 50)
    # Offset B so the combined centroid stays close to A's.
    a = np.outer(ta, a_dir) + rng.normal(scale=0.05, size=(500, 3))
    b = np.array([50.0, 0.0, 0.0]) + np.outer(tb, b_dir) + rng.normal(scale=0.05, size=(50, 3))
    lines = fit_lines(np.vstack([a, b]), n_lines=3,
                      distance_threshold_mm=0.3, min_inliers=20)
    assert len(lines) >= 1, f"expected at least 1 line, got {len(lines)}"
    v = np.array([lines[0].dx, lines[0].dy, lines[0].dz])
    v /= np.linalg.norm(v)
    assert abs(np.dot(v, a_dir)) > 0.99


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
