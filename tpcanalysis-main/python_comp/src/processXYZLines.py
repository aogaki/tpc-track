import pandas as pd
import numpy as np
from skimage.measure import LineModelND, ransac
from dataclasses import dataclass
from scipy.optimize import minimize


@dataclass
class modelDataXYZ:
    model_container: LineModelND = None
    min_val: float = None
    max_val: float = None


@dataclass
class modelDataXYZv2:
    x0: float = None
    y0: float = None
    z0: float = None
    dx: float = None
    dy: float = None
    dz: float = None
    min_lambda: float = None
    max_lambda: float = None


class processXYZLines:

    def __init__(self, entry):

        self._m_entry = entry

        self._m_features = self._m_entry[['x', 'y', 'z']].to_numpy()

        self._m_model_list = []

        self._m_model_list_v2 = []

    def setEntry(self, entry):

        self._m_entry = entry

        self._m_features = self._m_entry[['x', 'y', 'z']].to_numpy()

    def fitNLines(self, fig,  n, enable_scatter=True, min_samples=2, residual_threshold=3, max_trials=1000):

        self._m_model_list.clear()

        ax = fig.add_subplot(111, projection='3d')
        ax.set_xlim(-10, 150)
        ax.set_ylim(-10, 150)
        ax.set_zlim(-10, 150)

        features = self._m_features

        for i in range(n):

            if (len(features) > min_samples):

                model_var = modelDataXYZ()

                model_robust, inliers = ransac(features, LineModelND, min_samples=min_samples,
                                               residual_threshold=residual_threshold, max_trials=max_trials, random_state=0)

                model_var.model_container = model_robust

                outliers = inliers == False

                if (enable_scatter == True):
                    ax.scatter(features[inliers][:, 0], features[inliers][:, 1],
                               features[inliers][:, 2], c='b', marker='o', label=f'Inliers {i+1}', alpha=0.2)
                    ax.scatter(features[outliers][:, 0], features[outliers][:, 1],
                               features[outliers][:, 2], c='r', marker='o', label=f'Outliers {i+1}', alpha=0.2)

                line_points = np.linspace(features[inliers].min(
                    axis=0), features[inliers].max(axis=0), num=len(inliers))
                xyz_line = model_robust.predict(line_points[:, 0])
                ax.plot(xyz_line[:, 0], xyz_line[:, 1],
                        xyz_line[:, 2], label=f'Fitted line {i+1}')

                # Save the minimum and maximum values on x for each model

                model_var.min_val = features[inliers][:, 0].min()
                model_var.max_val = features[inliers][:, 0].max()

                self._m_model_list.append(model_var)

                # Update features to outliers for the next iteration
                features = features[outliers]
            else:
                print("Couldn't fit any more lines at i = ", i)

        ax.legend(loc='lower left')

        return self._m_model_list

    def line_distance3D(self, params, point):
        # https://www.nagwa.com/en/explainers/939127418581/
        # x0, y0, z0 is a point on the line
        # dx, dy, dz are the direction vector of the line
        x0, y0, z0, dx, dy, dz = params
        # Point coordinates
        x, y, z = point

        # Distance between our point and the point on the line.
        AP_x = x - x0
        AP_y = y - y0
        AP_z = z - z0

        # Cross product with the direction vector
        cp_x = AP_y * dz - AP_z * dy
        cp_y = AP_z * dx - AP_x * dz
        cp_z = AP_x * dy - AP_y * dx

        # Calculate the magnitude of the cross product
        mag_cp = np.sqrt(cp_x ** 2 + cp_y ** 2 + cp_z ** 2)

        # Calculate the magnitude of the direction vector
        mag_d = np.sqrt(dx ** 2 + dy ** 2 + dz ** 2)

        # The distance is the magnitude of the cross product over the magnitude of the direction vector
        distance = mag_cp / mag_d

        return distance

    def objective3D(self, params, points):
        total_distance = 0
        for point in points:
            total_distance += self.line_distance3D(params, point)
        return total_distance

    def findLambdaForPoint(self, x, x0, dx):
        lambda_val = (x - x0)/dx
        return lambda_val

    def fitNLinesv2(self, fig, n=2000, enable_scatter=True, min_samples=2, distance_threshold=3.5):

        self._m_model_list_v2.clear()

        ax = fig.add_subplot(111, projection='3d')
        ax.set_xlim(-10, 150)
        ax.set_ylim(-10, 150)
        ax.set_zlim(-10, 150)

        points = self._m_features

        if (enable_scatter):
            ax.scatter(points[:, 0], points[:, 1],
                       points[:, 2], color='green', label='Points')

        iter = 0

        while len(points) > min_samples and iter < n:

            # Perform optimization
            initial_guess3D = [0, 0, 0, 1, 1, 1]
            result = minimize(self.objective3D, initial_guess3D,
                              args=(points,), method='BFGS')
            x0, y0, z0, dx, dy, dz = result.x

            # Remove points belonging to the detected line
            distances = [self.line_distance3D(
                (x0, y0, z0, dx, dy, dz), point) for point in points]

            # the indices of the points to be removed
            points_to_remove = np.where(
                np.array(distances) <= distance_threshold)[0]

            min_x = np.min(points[points_to_remove][:, 0])
            max_x = np.max(points[points_to_remove][:, 0])

            lambda_min = self.findLambdaForPoint(min_x, x0, dx)
            lambda_max = self.findLambdaForPoint(max_x, x0, dx)

            loc_model = modelDataXYZv2(
                x0, y0, z0, dx, dy, dz, lambda_min, lambda_max)

            self._m_model_list_v2.append(loc_model)

            if (enable_scatter):
                ax.scatter(points[points_to_remove][:, 0], points[points_to_remove]
                           [:, 1], points[points_to_remove][:, 2], label=f'Set {iter+1}')

            t = np.linspace(lambda_min, lambda_max, 100)
            line_points_x = x0 + t * dx
            line_points_y = y0 + t * dy
            line_points_z = z0 + t * dz
            ax.plot(line_points_x, line_points_y, line_points_z,
                    color='r', label=f"Line {iter+1}")

            points = np.delete(points, points_to_remove, axis=0)
            iter += 1

        ax.legend(loc='lower left')

        return self._m_model_list_v2
