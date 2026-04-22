"""Iterative BFGS line fitting on 3D point clouds.

Fits up to `n_lines` straight lines to a set of (x, y, z) points by
minimising the sum of perpendicular distances, then removing the
inliers and refitting. Returns line parameters + endpoints for
rendering.

Design goals:
- No matplotlib coupling (unlike the upstream processXYZLines.py).
- Vectorised: the point-to-line distance and its sum over all points
  use numpy array ops so BFGS's finite-difference gradient stays fast
  even on ~20k points.
- Simple public API: pass in an (N, 3) array, get back a list of dicts.
"""
from __future__ import annotations

from dataclasses import dataclass
from typing import List

import numpy as np
from scipy.optimize import minimize


@dataclass
class Line3D:
    x0: float
    y0: float
    z0: float
    dx: float
    dy: float
    dz: float
    t_min: float
    t_max: float
    inlier_count: int

    def endpoints(self) -> np.ndarray:
        """Return the (2, 3) array of (start, end) XYZ points."""
        p0 = np.array([self.x0, self.y0, self.z0])
        d = np.array([self.dx, self.dy, self.dz])
        return np.stack([p0 + self.t_min * d, p0 + self.t_max * d])


def _point_line_distances(params: np.ndarray, points: np.ndarray) -> np.ndarray:
    """Perpendicular distance from each point to the line in `params`.

    params: [x0, y0, z0, dx, dy, dz]
    points: (N, 3)
    Returns (N,) array of distances in the same unit as `points`.
    """
    p0 = params[:3]
    d = params[3:]
    ap = points - p0
    cross = np.cross(ap, d)
    num = np.linalg.norm(cross, axis=1)
    den = np.linalg.norm(d)
    if den < 1e-12:
        return np.full(len(points), np.inf)
    return num / den


def _objective(params: np.ndarray, points: np.ndarray) -> float:
    return float(_point_line_distances(params, points).sum())


def _pca_initial_guess(points: np.ndarray) -> np.ndarray:
    """Best-fit line in one shot: centroid + principal-axis direction.

    This gives BFGS a much better starting point than a fixed
    `(0, 0, 0, 1, 1, 1)` when the remaining points don't straddle the
    origin — which is normally the case after the first iteration has
    peeled off the dominant track.
    """
    centre = points.mean(axis=0)
    centred = points - centre
    # Covariance matrix is (3, 3); its top eigenvector is the line direction
    # that minimises sum of squared perpendicular distances.
    cov = centred.T @ centred / max(len(points), 1)
    eigvals, eigvecs = np.linalg.eigh(cov)
    direction = eigvecs[:, -1]  # column matching largest eigenvalue
    return np.concatenate([centre, direction])


def fit_lines(
    points: np.ndarray,
    n_lines: int = 3,
    distance_threshold_mm: float = 3.0,
    min_inliers: int = 20,
) -> List[Line3D]:
    """Iteratively fit up to `n_lines` lines to `points` (N, 3).

    Each iteration:
      1. Use PCA on the remaining points to get a good initial guess.
      2. BFGS-minimise the sum of perpendicular distances from there.
      3. Mark points within `distance_threshold_mm` as inliers.
      4. Bail out if fewer than `min_inliers` inliers were captured.
      5. Parameterise the inliers along the line to get (t_min, t_max)
         so an overlay endpoint can be drawn.
      6. Remove the inliers from the remaining set and repeat.
    """
    if points.ndim != 2 or points.shape[1] != 3:
        raise ValueError("points must be shape (N, 3)")

    remaining = points.astype(float, copy=True)
    out: List[Line3D] = []
    for _ in range(n_lines):
        if len(remaining) < min_inliers:
            break
        x0_guess = _pca_initial_guess(remaining)
        res = minimize(_objective, x0_guess, args=(remaining,), method="BFGS")
        dists = _point_line_distances(res.x, remaining)
        inlier_mask = dists <= distance_threshold_mm
        n_inliers = int(inlier_mask.sum())
        if n_inliers < min_inliers:
            break

        x0, y0, z0, dx, dy, dz = res.x
        p0 = np.array([x0, y0, z0])
        d = np.array([dx, dy, dz])
        d_norm_sq = float(np.dot(d, d))
        inliers = remaining[inlier_mask]
        ts = np.dot(inliers - p0, d) / d_norm_sq
        t_min, t_max = float(ts.min()), float(ts.max())

        out.append(
            Line3D(
                x0=float(x0), y0=float(y0), z0=float(z0),
                dx=float(dx), dy=float(dy), dz=float(dz),
                t_min=t_min, t_max=t_max, inlier_count=n_inliers,
            )
        )
        remaining = remaining[~inlier_mask]
    return out
