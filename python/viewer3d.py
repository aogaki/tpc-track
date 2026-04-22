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


def voxelize(
    df: pd.DataFrame, voxel_size_mm: float = 2.0, min_count: int = 3
) -> pd.DataFrame:
    """Collapse `df` into voxels and drop those with fewer than `min_count` hits.

    Real ionization points on 3 planes produce many (u, v, w) cartesian combos
    that all map back to the same (x, y, z) voxel. Ghost combos scatter to
    different voxels. Requiring >= `min_count` combos per voxel kills most
    ghosts while preserving real tracks.

    Returns one row per surviving voxel with columns:
        event_id, x_mm, y_mm, z_mm (voxel center), charge (sum), count.
    """
    if voxel_size_mm <= 0:
        return df.assign(count=1)

    g = df.assign(
        _vx=np.round(df["x_mm"] / voxel_size_mm).astype(np.int32),
        _vy=np.round(df["y_mm"] / voxel_size_mm).astype(np.int32),
        _vz=np.round(df["z_mm"] / voxel_size_mm).astype(np.int32),
    )
    agg = (
        g.groupby(["event_id", "_vx", "_vy", "_vz"], sort=False)
        .agg(charge=("charge", "sum"), count=("charge", "size"))
        .reset_index()
    )
    agg = agg[agg["count"] >= min_count].copy()
    agg["x_mm"] = agg["_vx"] * voxel_size_mm
    agg["y_mm"] = agg["_vy"] * voxel_size_mm
    agg["z_mm"] = agg["_vz"] * voxel_size_mm
    return agg[["event_id", "x_mm", "y_mm", "z_mm", "charge", "count"]]


def plot_event(
    df: pd.DataFrame,
    event_id: int,
    voxel_size_mm: float = 2.0,
    min_count: int = 3,
    max_points: int = 20_000,
    marker_size: int = 2,
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

    if voxel_size_mm > 0:
        ev = voxelize(ev_raw, voxel_size_mm=voxel_size_mm, min_count=min_count)
    else:
        ev = ev_raw.assign(count=1)

    if len(ev) > max_points:
        ev = ev.nlargest(max_points, "count")
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
                    color=np.log10(ev["count"].clip(lower=1)),
                    colorscale="Viridis",
                    colorbar=dict(title="log10(voxel count)"),
                ),
                customdata=np.stack(
                    [ev["charge"].to_numpy(), ev["count"].to_numpy()],
                    axis=-1,
                ),
                hovertemplate=(
                    "x=%{x:.1f} mm<br>y=%{y:.1f} mm<br>z=%{z:.1f} mm<br>"
                    "charge=%{customdata[0]:.0f}<br>count=%{customdata[1]}"
                    "<extra></extra>"
                ),
            )
        ]
    )
    fig.update_layout(
        title=title
        or (
            f"event {event_id}  "
            f"raw={raw_total}  voxelized={shown}  "
            f"(voxel={voxel_size_mm} mm, min_count={min_count})"
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
