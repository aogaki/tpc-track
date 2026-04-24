"""Iterative SVD line fitting on 3D point clouds.

Fits up to `n_lines` straight lines to a set of (x, y, z) points by
closed-form L2 (perpendicular distance) minimisation via SVD, then
peeling inliers and refitting. Returns line parameters + endpoints for
rendering.

Design goals:
- Closed-form per iteration: one SVD call, no numerical optimiser.
  ~100x faster than the previous BFGS version, which matters when
  `fit_all_events` processes thousands of events.
- No matplotlib / upstream coupling.
- Vectorised numpy throughout.
"""
from __future__ import annotations

from dataclasses import dataclass
from typing import Callable, List, Optional, Tuple

import numpy as np
import pandas as pd


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


def fit_lines(
    points: np.ndarray,
    n_lines: int = 3,
    distance_threshold_mm: float = 3.0,
    min_inliers: int = 20,
) -> List[Line3D]:
    """Iteratively fit up to `n_lines` lines to `points` (N, 3).

    Each iteration:
      1. SVD of the centred remaining points → best-fit line (L2 closed form).
      2. Mark points within `distance_threshold_mm` as inliers.
      3. Bail out if fewer than `min_inliers` inliers were captured.
      4. Parameterise the inliers along the line to get (t_min, t_max).
      5. Peel the inliers and repeat.

    PCA/SVD minimises the sum of **squared** perpendicular distances and
    is the maximum-likelihood estimator for isotropic Gaussian noise.
    It loses a bit of outlier robustness compared to L1, but after the
    voxel-coincidence and charge-ratio filters have already killed
    ghosts, the remaining points are clean enough that L2 wins on speed
    (~100x over BFGS) with no practical accuracy hit.
    """
    if points.ndim != 2 or points.shape[1] != 3:
        raise ValueError("points must be shape (N, 3)")

    remaining = points.astype(float, copy=True)
    out: List[Line3D] = []
    for _ in range(n_lines):
        if len(remaining) < min_inliers:
            break
        centre = remaining.mean(axis=0)
        centred = remaining - centre
        # Right-singular vectors; the first one is the unit principal axis.
        _, _, vh = np.linalg.svd(centred, full_matrices=False)
        d = vh[0]

        # |centred × d|  (d is unit, so no /|d|)
        dists = np.linalg.norm(np.cross(centred, d), axis=1)
        inlier_mask = dists <= distance_threshold_mm
        n_inliers = int(inlier_mask.sum())
        if n_inliers < min_inliers:
            break

        # Project inliers onto the line to get t_min / t_max.
        ts = centred[inlier_mask] @ d
        t_min, t_max = float(ts.min()), float(ts.max())

        out.append(
            Line3D(
                x0=float(centre[0]), y0=float(centre[1]), z0=float(centre[2]),
                dx=float(d[0]), dy=float(d[1]), dz=float(d[2]),
                t_min=t_min, t_max=t_max, inlier_count=n_inliers,
            )
        )
        remaining = remaining[~inlier_mask]
    return out


def fit_all_events(
    df: pd.DataFrame,
    filter_fn: Callable[[pd.DataFrame], pd.DataFrame],
    n_lines: int = 3,
    distance_threshold_mm: float = 3.0,
    min_inliers: int = 20,
    progress: Optional[Callable[[int, int], None]] = None,
) -> List[Tuple[int, Line3D]]:
    """Run `fit_lines` on every event in `df` and return (event_id, line) pairs.

    Parameters
    ----------
    df : DataFrame with columns event_id, x_mm, y_mm, z_mm, charge.
    filter_fn : function taking the per-event DataFrame and returning the
        point subset to feed the fitter. The viewer passes its voxel filter
        here so batch fits see the same points the interactive view does.
    n_lines, distance_threshold_mm, min_inliers : forwarded to fit_lines.
    progress : optional callback(done, total) for UI updates.
    """
    event_ids = sorted(df["event_id"].unique().tolist())
    total = len(event_ids)
    out: List[Tuple[int, Line3D]] = []
    for i, eid in enumerate(event_ids):
        ev = df[df["event_id"] == eid]
        ev = filter_fn(ev)
        if len(ev) < min_inliers:
            if progress:
                progress(i + 1, total)
            continue
        pts = ev[["x_mm", "y_mm", "z_mm"]].to_numpy()
        for line in fit_lines(
            pts,
            n_lines=n_lines,
            distance_threshold_mm=distance_threshold_mm,
            min_inliers=min_inliers,
        ):
            out.append((int(eid), line))
        if progress:
            progress(i + 1, total)
    return out
