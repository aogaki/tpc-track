import pandas as pd
import numpy as np
from skimage.measure import LineModelND, ransac
import matplotlib.pyplot as plt
from dataclasses import dataclass
from scipy.optimize import minimize


@dataclass
class modelDataUVW:
    model_container: LineModelND = None
    min_val: float = None
    max_val: float = None
    slope: float = None
    intercept: float = None
    plane: int = None

    def calculateSlopeIntercept(self):
        self.slope = self.model_container.params[1][1] / \
            self.model_container.params[1][0]
        self.intercept = self.model_container.params[0][1] - \
            self.slope * self.model_container.params[0][0]


@dataclass
class modelDataUVWHough:
    m: float = None
    b: float = None
    rho: float = None
    theta: float = None
    min_val: float = None
    max_val: float = None
    plane: int = None

    def calculateSlopeIntercept(self):
        if (self.theta == 0):
            self.m = None
            self.b = None
        else:
            self.m = -np.cos(self.theta) / np.sin(self.theta)  # Slope
            self.b = self.rho / np.sin(self.theta)  # Intercept


class processUVWLines:
    def __init__(self, entry):
        self._m_entry = entry
        self._m_hits = []

        # The index of the array will be equivalent to the value of the plane.
        for i in range(3):
            temp_entry = entry[entry['plane'] == i]
            self._m_hits.append(temp_entry[['time_bin', 'strip']].to_numpy())

        # The notation for each plane.
        self._m_plane_notations = {
            0: "U",
            1: "V",
            2: "W"
        }

        self._m_model_list = []
        self._m_model_hough_list = []

    def setEntry(self, entry):
        self._m_entry = entry

        for i in range(3):
            temp_entry = entry[entry['plane'] == i]
            self._m_hits.append(temp_entry[['time_bin', 'strip']].to_numpy())

    def drawHits(self, axes):

        for i in range(3):
            axes[i].scatter(self._m_hits[i][:, 0], self._m_hits[i][:, 1])
            axes[i].set_title('Plane ' + self._m_plane_notations)
            axes[i].set_xlim(-10, 520)
            axes[i].set_ylim(-10, 100)

    # RANSAC version

    def _fitLinesForPlane(self, features, plane, axes,  n, enable_scatter=True, min_samples=2, residual_threshold=3, max_trials=1000):

        axes[plane].set_title('Plane ' + self._m_plane_notations[plane])
        axes[plane].set_xlim(-10, 520)
        axes[plane].set_ylim(-10, 100)
        axes[plane].grid(True)

        for i in range(n):

            if (len(features) > min_samples):

                model_var = modelDataUVW()

                model_var.plane = plane

                model, inliers = ransac(features, LineModelND, min_samples=min_samples,
                                        residual_threshold=residual_threshold, max_trials=max_trials, random_state=0)

                model_var.model_container = model

                outliers = inliers == False

                if (enable_scatter == True):
                    axes[plane].scatter(features[inliers][:, 0], features[inliers][:, 1],
                                        c='b', marker='o', label=f'Inliers {i+1}', alpha=0.2)
                    axes[plane].scatter(features[outliers][:, 0], features[outliers][:, 1],
                                        c='r', marker='o', label=f'Outliers {i+1}', alpha=0.2)

                line_points = np.linspace(features[inliers].min(
                    axis=0), features[inliers].max(axis=0), num=len(inliers))
                xy_line = model.predict(line_points[:, 0])
                axes[plane].plot(xy_line[:, 0], xy_line[:, 1],
                                 label=f'Fitted line {i+1}')

                # Save the minimum and maximum values on x for each model.

                model_var.min_val = features[inliers][:, 0].min()
                model_var.max_val = features[inliers][:, 0].max()

                # Calculates slope and intercept based on the model.
                model_var.calculateSlopeIntercept()

                self._m_model_list.append(model_var)

                # Update features to outliers for the next iteration
                features = features[outliers]
            else:
                print("Couldn't fit any more lines for plane " + self._m_plane_notations[plane] +
                      " at i = ", i)

        axes[plane].legend(loc='best')

    def fitNLines(self, axes,  n, enable_scatter=True, min_samples=2, residual_threshold=3, max_trials=1000):

        self._m_model_list.clear()

        for i in range(3):
            self._fitLinesForPlane(
                self._m_hits[i], i, axes, n, enable_scatter, min_samples, residual_threshold, max_trials)

        return self._m_model_list

    # Hough transform version

    def hessian_distance(self, params, point):
        rho, theta = params
        x, y = point
        return np.abs(x * np.cos(theta) + y * np.sin(theta) - rho)

    def objective(self, params, points):
        total_distance = 0
        for point in points:
            total_distance += self.hessian_distance(params, point)
        return total_distance

    def _fitHoughLinesForPlane(self, features, plane, axes,  n, enable_scatter=True, distance_threshold=3, min_samples=2):

        axes[plane].set_title('Plane ' + self._m_plane_notations[plane])
        axes[plane].set_xlim(-10, 520)
        axes[plane].set_ylim(-10, 100)
        axes[plane].grid(True)

        iter = 0
        while len(features) > min_samples and iter < n:

            loc_hough_model = modelDataUVWHough()

            loc_hough_model.plane = plane

            # Perform optimization
            initial_guess = [0, 0]  # Initial guess for rho and theta
            result = minimize(self.objective, initial_guess,
                              args=(features,), method='BFGS')
            rho, theta = result.x

            loc_hough_model.rho = rho
            loc_hough_model.theta = theta

            """ # Calculate line equation
            m = -np.cos(theta) / np.sin(theta)  # Slope
            b = rho / np.sin(theta)             # Intercept

            loc_hough_model.m = m
            loc_hough_model.b = b """

            loc_hough_model.calculateSlopeIntercept()

            # Remove points belonging to the detected line
            distances = [self.hessian_distance((rho, theta), point)
                         for point in features]

            # the indices of the points to be removed
            points_to_remove = np.where(
                np.array(distances) <= distance_threshold)[0]

            # Use the min and max values on x to find where the line starts and ends.
            if (len(points_to_remove) == 0):
                loc_hough_model.min_val = 0
                loc_hough_model.max_val = 0
            else:
                loc_hough_model.min_val = np.min(
                    features[points_to_remove][:, 0])
                loc_hough_model.max_val = np.max(
                    features[points_to_remove][:, 0])

            x = np.array([loc_hough_model.min_val, loc_hough_model.max_val])

            if (loc_hough_model.m != None and loc_hough_model.b != None):
                y = loc_hough_model.m * x + loc_hough_model.b
                axes[plane].plot(
                    x, y, c='r', label=f"Line {iter+1}: y = {loc_hough_model.m}x + {loc_hough_model.b}")
            else:
                # in case of vertical line
                if (len(points_to_remove) == 0):
                    break
                else:
                    y = np.min(
                        features[points_to_remove][:, 1])
                axes[plane].axvline(
                    x=x[0], y=y, color='r', linestyle='--', label=f"Line {iter+1}: vertical")

            if (enable_scatter):
                axes[plane].scatter(features[points_to_remove, 0],
                                    features[points_to_remove, 1], label=f'Set {iter+1}')

            features = np.delete(features, points_to_remove, axis=0)
            iter += 1

            self._m_model_hough_list.append(loc_hough_model)

        if (enable_scatter):
            axes[plane].scatter(features[:, 0], features[:, 1],
                                c='k', label='Remaining Points')

        axes[plane].legend(loc='best')

    def fitNLinesHough(self, axes, n, enable_scatter=True, distance_threshold=3, min_samples=2):

        self._m_model_hough_list.clear()

        for i in range(3):
            self._fitHoughLinesForPlane(
                self._m_hits[i], i, axes, n, enable_scatter, distance_threshold, min_samples)

        return self._m_model_hough_list
