"""Plotly 3D viewer for one TPC event.

Loads the point-cloud CSV produced by tpcanalysis-main/run_mini.cpp and
renders a single event as an interactive ``scatter_3d`` figure.

The CSV from M5 can contain >10^5 points per event due to cartesian
ghost combinations. `plot_event` downsamples to `max_points` (default
20_000) by picking the highest-charge points so the browser stays
responsive.
"""
from __future__ import annotations

from pathlib import Path
from typing import Iterable, Optional

import numpy as np
import pandas as pd
import plotly.graph_objects as go

from line_fit import Line3D, fit_all_events, fit_lines  # noqa: F401


def load_points(csv_path: str | Path) -> pd.DataFrame:
    """Load the run_mini points CSV as a DataFrame.

    Expected columns: event_id, x_mm, y_mm, z_mm, charge.
    """
    df = pd.read_csv(csv_path)
    required = {"event_id", "x_mm", "y_mm", "z_mm", "charge"}
    missing = required - set(df.columns)
    if missing:
        raise ValueError(f"CSV is missing columns: {missing}")
    return df


def filter_dense_voxels(
    df: pd.DataFrame, voxel_size_mm: float = 2.0, min_count: int = 3
) -> pd.DataFrame:
    """Keep rows that fall into voxels containing >= min_count points.

    Real ionization points on 3 planes produce many (u, v, w) cartesian combos
    that all map back to the same (x, y, z) voxel. Ghost combos scatter to
    different voxels. Requiring >= `min_count` combos per voxel kills most
    ghosts while preserving the original point granularity for rendering.

    Returns a subset of `df` (same columns) with the rejected rows dropped.
    """
    if voxel_size_mm <= 0 or min_count <= 1:
        return df

    g = df.assign(
        _vx=np.round(df["x_mm"] / voxel_size_mm).astype(np.int32),
        _vy=np.round(df["y_mm"] / voxel_size_mm).astype(np.int32),
        _vz=np.round(df["z_mm"] / voxel_size_mm).astype(np.int32),
    )
    counts = g.groupby(["_vx", "_vy", "_vz"])["x_mm"].transform("size")
    return df[counts >= min_count]


def overlay_lines(fig: go.Figure, lines: Iterable[Line3D]) -> go.Figure:
    """Add each fitted line as a Scatter3d line trace on top of `fig`."""
    palette = ["#e6194B", "#f58231", "#ffe119", "#3cb44b", "#4363d8", "#911eb4"]
    for i, line in enumerate(lines):
        (x0, y0, z0), (x1, y1, z1) = line.endpoints()
        color = palette[i % len(palette)]
        fig.add_trace(
            go.Scatter3d(
                x=[x0, x1], y=[y0, y1], z=[z0, z1],
                mode="lines",
                line=dict(color=color, width=6),
                name=f"line {i} (n={line.inlier_count})",
            )
        )
    return fig


def plot_event(
    df: pd.DataFrame,
    event_id: int,
    voxel_size_mm: float = 0.0,
    min_count: int = 1,
    max_points: int = 20_000,
    marker_size: int = 2,
    n_lines: int = 0,
    fit_threshold_mm: float = 3.0,
    fit_min_inliers: int = 20,
    title: Optional[str] = None,
) -> go.Figure:
    """Return a Plotly Figure for one event.

    Parameters
    ----------
    df : DataFrame
        Whole-file point cloud (output of `load_points`).
    event_id : int
        Which event to render.
    voxel_size_mm : float
        Voxel edge length in mm. Set to 0 to disable voxel filtering.
    min_count : int
        Keep voxels with this many (u, v, w) combinations or more. Higher
        values reject more ghosts at the cost of real-hit sensitivity.
    max_points : int
        Safety cap: if voxelized data still exceeds this, keep the
        highest-count voxels. Ensures Plotly stays responsive.
    marker_size : int
        Plotly marker size (px). Small values keep the cloud readable.
    title : str
        Optional title. Defaults to a summary of event_id and point count.
    """
    ev_raw = df[df["event_id"] == event_id]
    if ev_raw.empty:
        raise ValueError(f"event_id={event_id} not found in DataFrame")
    raw_total = len(ev_raw)

    ev = filter_dense_voxels(
        ev_raw, voxel_size_mm=voxel_size_mm, min_count=min_count
    )
    kept = len(ev)

    if kept > max_points:
        ev = ev.nlargest(max_points, "charge")
    shown = len(ev)

    fig = go.Figure(
        data=[
            go.Scatter3d(
                x=ev["x_mm"],
                y=ev["y_mm"],
                z=ev["z_mm"],
                mode="markers",
                marker=dict(
                    size=marker_size,
                    color=np.log10(ev["charge"].clip(lower=1.0)),
                    colorscale="Viridis",
                    colorbar=dict(title="log10(charge)"),
                ),
                name="hits",
            )
        ]
    )

    n_fit = 0
    if n_lines > 0 and shown > 0:
        pts_xyz = ev[["x_mm", "y_mm", "z_mm"]].to_numpy()
        lines = fit_lines(
            pts_xyz,
            n_lines=n_lines,
            distance_threshold_mm=fit_threshold_mm,
            min_inliers=fit_min_inliers,
        )
        overlay_lines(fig, lines)
        n_fit = len(lines)

    fig.update_layout(
        title=title
        or (
            f"event {event_id}  "
            f"raw={raw_total}  after filter={kept}  shown={shown}"
            + (f"  lines fit={n_fit}/{n_lines}" if n_lines > 0 else "")
            + f"  (voxel={voxel_size_mm} mm, min_count={min_count})"
        ),
        scene=dict(
            xaxis_title="x [mm]",
            yaxis_title="y [mm]",
            zaxis_title="z [mm]",
            aspectmode="data",
        ),
        width=900,
        height=700,
    )
    return fig


def plot_all_lines(
    lines_with_eid,
    color_by: str = "event_id",
    opacity: float = 0.4,
    line_width: int = 2,
    title: Optional[str] = None,
) -> go.Figure:
    """Render every fitted line from a batch fit in a single 3D figure.

    Parameters
    ----------
    lines_with_eid : iterable of (event_id:int, Line3D)
        Output of `line_fit.fit_all_events`.
    color_by : "event_id" | "inliers" | "none"
        "event_id" maps the Viridis palette onto chronological event order,
        so a run's time evolution is visible.
        "inliers" colours by fit quality — saturated yellow = best fit.
        "none" uses a single mid-blue for every line; distinguishes density
        via `opacity` only.
    opacity : 0..1
        Per-line alpha. Lower it for 1000+-line plots so the inside of the
        detector stays legible.
    line_width : px
    """
    pairs = list(lines_with_eid)
    if not pairs:
        fig = go.Figure()
        fig.update_layout(title="no lines fit")
        return fig

    eids = np.array([eid for eid, _ in pairs], dtype=float)
    inliers = np.array([ln.inlier_count for _, ln in pairs], dtype=float)

    if color_by == "event_id":
        denom = eids.max() - eids.min() or 1.0
        t = (eids - eids.min()) / denom
        colorbar_title = "event_id"
        values = eids
    elif color_by == "inliers":
        denom = inliers.max() - inliers.min() or 1.0
        t = (inliers - inliers.min()) / denom
        colorbar_title = "inlier count"
        values = inliers
    else:
        t = np.full_like(eids, 0.5)
        colorbar_title = None
        values = None

    # Sample Viridis colours once (avoids 1000 separate colorscale lookups).
    from plotly.colors import sample_colorscale
    colors = sample_colorscale("Viridis", t.tolist())

    # Build ONE Scatter3d trace with None-separated segments so Plotly can
    # draw 1000+ lines without choking on 1000+ traces.
    xs, ys, zs = [], [], []
    seg_colors = []
    for (eid, line), col in zip(pairs, colors):
        (x0, y0, z0), (x1, y1, z1) = line.endpoints()
        xs.extend([x0, x1, None])
        ys.extend([y0, y1, None])
        zs.extend([z0, z1, None])
        seg_colors.extend([col, col, col])

    traces = [
        go.Scatter3d(
            x=xs, y=ys, z=zs,
            mode="lines",
            line=dict(width=line_width, color=seg_colors),
            opacity=opacity,
            name=f"{len(pairs)} lines",
            showlegend=False,
        )
    ]
    # Add an invisible marker trace just to show a colorbar when applicable.
    if values is not None:
        traces.append(
            go.Scatter3d(
                x=[None], y=[None], z=[None],
                mode="markers",
                marker=dict(
                    color=values, colorscale="Viridis",
                    cmin=float(values.min()), cmax=float(values.max()),
                    colorbar=dict(title=colorbar_title),
                    size=0.1, opacity=0,
                ),
                showlegend=False,
            )
        )

    fig = go.Figure(data=traces)
    fig.update_layout(
        title=title or f"{len(pairs)} fitted lines over {len(set(eids.astype(int)))} events",
        scene=dict(
            xaxis_title="x [mm]",
            yaxis_title="y [mm]",
            zaxis_title="z [mm]",
            aspectmode="data",
        ),
        width=900,
        height=700,
    )
    return fig
