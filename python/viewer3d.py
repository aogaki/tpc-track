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
from typing import Optional

import numpy as np
import pandas as pd
import plotly.graph_objects as go


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


def plot_event(
    df: pd.DataFrame,
    event_id: int,
    voxel_size_mm: float = 0.0,
    min_count: int = 1,
    max_points: int = 20_000,
    marker_size: int = 2,
    xy_range_mm: float = 120.0,
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
            )
        ]
    )
    fig.update_layout(
        title=title
        or (
            f"event {event_id}  "
            f"raw={raw_total}  after filter={kept}  shown={shown}  "
            f"(voxel={voxel_size_mm} mm, min_count={min_count})"
        ),
        scene=dict(
            xaxis=dict(title="x [mm]", range=[-xy_range_mm, xy_range_mm]),
            yaxis=dict(title="y [mm]", range=[-xy_range_mm, xy_range_mm]),
            zaxis=dict(title="z [mm]"),
            aspectmode="cube",
        ),
        width=900,
        height=700,
    )
    return fig
