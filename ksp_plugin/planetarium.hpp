﻿
#pragma once

#include <vector>

#include "base/not_null.hpp"
#include "geometry/named_quantities.hpp"
#include "geometry/orthogonal_map.hpp"
#include "geometry/perspective.hpp"
#include "geometry/rp2_point.hpp"
#include "geometry/sphere.hpp"
#include "ksp_plugin/frames.hpp"
#include "physics/degrees_of_freedom.hpp"
#include "physics/discrete_trajectory.hpp"
#include "physics/ephemeris.hpp"
#include "physics/rigid_motion.hpp"
#include "quantities/quantities.hpp"

namespace principia {
namespace ksp_plugin {
namespace internal_planetarium {

using base::not_null;
using geometry::Displacement;
using geometry::Instant;
using geometry::OrthogonalMap;
using geometry::Perspective;
using geometry::Position;
using geometry::R3Element;
using geometry::RP2Lines;
using geometry::RP2Point;
using geometry::Segment;
using geometry::Segments;
using geometry::Sphere;
using physics::DegreesOfFreedom;
using physics::DiscreteTrajectory;
using physics::Ephemeris;
using physics::RigidMotion;
using physics::Trajectory;
using quantities::Angle;
using quantities::Length;

// Corresponds to a UnityEngine.Vector3 representing a position in KSP’s
// ScaledSpace.
extern "C" struct ScaledSpacePoint {
  static inline ScaledSpacePoint FromCoordinates(
      R3Element<double> const& coordinates);

  float x;
  float y;
  float z;
};

static_assert(std::is_pod<ScaledSpacePoint>::value,
              "NavigationFrameParameters is used for interfacing");

// A planetarium is an ephemeris together with a perspective.  In this setting
// it is possible to draw trajectories in the projective plane.
class Planetarium {
 public:
  class Parameters final {
   public:
    // |sphere_radius_multiplier| defines the "dark area" around a celestial
    // where we don't draw trajectories.  |angular_resolution| defines the limit
    // beyond which spheres don't participate in hiding.  |field_of_view|
    // is the half-angle of a cone outside of which not plotting takes place.
    explicit Parameters(double sphere_radius_multiplier,
                        Angle const& angular_resolution,
                        Angle const& field_of_view);

   private:
    double const sphere_radius_multiplier_;
    double const sin²_angular_resolution_;
    double const tan_angular_resolution_;
    double const tan_field_of_view_;
    friend class Planetarium;
  };

  using PlottingToScaledSpaceConversion =
      std::function<ScaledSpacePoint(Position<Navigation> const&)>;

  // TODO(phl): All this Navigation is weird.  Should it be named Plotting?
  // In particular Navigation vs. NavigationFrame is a mess.
  Planetarium(Parameters const& parameters,
              Perspective<Navigation, Camera> perspective,
              not_null<Ephemeris<Barycentric> const*> ephemeris,
              not_null<NavigationFrame const*> plotting_frame,
              PlottingToScaledSpaceConversion plotting_to_scaled_space);

  // A no-op method that just returns all the points in the trajectory defined
  // by |begin| and |end|.
  RP2Lines<Length, Camera> PlotMethod0(
      DiscreteTrajectory<Barycentric> const& trajectory,
      DiscreteTrajectory<Barycentric>::iterator begin,
      DiscreteTrajectory<Barycentric>::iterator end,
      Instant const& now,
      bool reverse) const;

  // A method that coalesces segments until they are larger than the angular
  // resolution.
  RP2Lines<Length, Camera> PlotMethod1(
      DiscreteTrajectory<Barycentric> const& trajectory,
      DiscreteTrajectory<Barycentric>::iterator begin,
      DiscreteTrajectory<Barycentric>::iterator end,
      Instant const& now,
      bool reverse) const;

  // A method that plots the cubic Hermite spline interpolating the trajectory,
  // using an adaptive step size to keep the error between the straight segments
  // and the actual spline below and close to the angular resolution.
  RP2Lines<Length, Camera> PlotMethod2(
      Trajectory<Barycentric> const& trajectory,
      DiscreteTrajectory<Barycentric>::iterator begin,
      DiscreteTrajectory<Barycentric>::iterator end,
      Instant const& now,
      bool reverse) const;

  // The same method, operating on the |Trajectory| interface.
  RP2Lines<Length, Camera> PlotMethod2(
      Trajectory<Barycentric> const& trajectory,
      Instant const& first_time,
      Instant const& last_time,
      Instant const& now,
      bool reverse,
      Length* minimal_distance = nullptr) const;

  // A method similar to PlotMethod2, but which produces a three-dimensional
  // trajectory in scaled space instead of projecting and hiding.
  void PlotMethod3(
      Trajectory<Barycentric> const& trajectory,
      DiscreteTrajectory<Barycentric>::iterator begin,
      DiscreteTrajectory<Barycentric>::iterator end,
      Instant const& now,
      bool reverse,
      std::function<void(ScaledSpacePoint const&)> const& add_point,
      int max_points) const;

  // The same method, operating on the |Trajectory| interface.
  void PlotMethod3(
      Trajectory<Barycentric> const& trajectory,
      Instant const& first_time,
      Instant const& last_time,
      Instant const& now,
      bool reverse,
      std::function<void(ScaledSpacePoint const&)> const& add_point,
      int max_points,
      Length* minimal_distance = nullptr) const;

 private:
  // Computes the coordinates of the spheres that represent the |ephemeris_|
  // bodies.  These coordinates are in the |plotting_frame_| at time |now|.
  std::vector<Sphere<Navigation>> ComputePlottableSpheres(
      Instant const& now) const;

  // Computes the segments of the trajectory defined by |begin| and |end| that
  // are not hidden by the |plottable_spheres|.
  Segments<Navigation> ComputePlottableSegments(
      const std::vector<Sphere<Navigation>>& plottable_spheres,
      DiscreteTrajectory<Barycentric>::iterator begin,
      DiscreteTrajectory<Barycentric>::iterator end) const;

  Parameters const parameters_;
  Perspective<Navigation, Camera> const perspective_;
  not_null<Ephemeris<Barycentric> const*> const ephemeris_;
  not_null<NavigationFrame const*> const plotting_frame_;
  PlottingToScaledSpaceConversion plotting_to_scaled_space_;
};

inline ScaledSpacePoint ScaledSpacePoint::FromCoordinates(
    R3Element<double> const& coordinates) {
  return ScaledSpacePoint{static_cast<float>(coordinates.x),
                          static_cast<float>(coordinates.y),
                          static_cast<float>(coordinates.z)};
}

}  // namespace internal_planetarium

using internal_planetarium::Planetarium;
using internal_planetarium::ScaledSpacePoint;

}  // namespace ksp_plugin
}  // namespace principia
