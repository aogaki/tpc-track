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


def plot_event(
    df: pd.DataFrame,
    event_id: int,
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
    max_points : int
        If the event has more than this many points, keep the highest-
        charge ones. Avoids choking the browser on ghost-heavy events.
    marker_size : int
        Plotly marker size (px). Small values keep the cloud readable.
    title : str
        Optional title. Defaults to a summary of event_id and point count.
    """
    ev = df[df["event_id"] == event_id]
    if ev.empty:
        raise ValueError(f"event_id={event_id} not found in DataFrame")

    total = len(ev)
    if total > max_points:
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
        or f"event {event_id}  ({shown} / {total} points shown)",
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
