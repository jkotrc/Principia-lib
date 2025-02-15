﻿
#include "ksp_plugin/renderer.hpp"

#include <algorithm>
#include <optional>

#include "geometry/grassmann.hpp"
#include "geometry/named_quantities.hpp"
#include "physics/apsides.hpp"
#include "physics/body_centred_body_direction_dynamic_frame.hpp"
#include "physics/degrees_of_freedom.hpp"

namespace principia {
namespace ksp_plugin {
namespace internal_renderer {

using base::make_not_null_unique;
using geometry::AngularVelocity;
using geometry::OddPermutation;
using geometry::Permutation;
using geometry::RigidTransformation;
using geometry::Vector;
using geometry::Velocity;
using physics::BodyCentredBodyDirectionDynamicFrame;
using physics::DegreesOfFreedom;

Renderer::Renderer(not_null<Celestial const*> const sun,
                   not_null<std::unique_ptr<NavigationFrame>> plotting_frame)
    : sun_(sun),
      plotting_frame_(std::move(plotting_frame)) {}

void Renderer::SetPlottingFrame(
    not_null<std::unique_ptr<NavigationFrame>> plotting_frame) {
  plotting_frame_ = std::move(plotting_frame);
}

not_null<NavigationFrame const*> Renderer::GetPlottingFrame() const {
  return target_ ? target_->target_frame.get()
                 : plotting_frame_.get();
}

void Renderer::SetTargetVessel(
    not_null<Vessel*> const vessel,
    not_null<Celestial const*> const celestial,
    not_null<Ephemeris<Barycentric> const*> const ephemeris) {
  if (!target_ ||
      target_->vessel != vessel ||
      target_->celestial != celestial) {
    target_.emplace(vessel, celestial, ephemeris);
  }
}

void Renderer::ClearTargetVessel() {
  target_ = std::nullopt;
}

void Renderer::ClearTargetVesselIf(not_null<Vessel*> const vessel) {
  if (target_ && target_->vessel == vessel) {
    target_ = std::nullopt;
  }
}

bool Renderer::HasTargetVessel() const {
  return static_cast<bool>(target_);
}

Vessel& Renderer::GetTargetVessel() {
  CHECK(target_);
  return *target_->vessel;
}

Vessel const& Renderer::GetTargetVessel() const {
  CHECK(target_);
  return *target_->vessel;
}

DiscreteTrajectory<World>
Renderer::RenderBarycentricTrajectoryInWorld(
    Instant const& time,
    DiscreteTrajectory<Barycentric>::iterator const& begin,
    DiscreteTrajectory<Barycentric>::iterator const& end,
    Position<World> const& sun_world_position,
    Rotation<Barycentric, AliceSun> const& planetarium_rotation) const {
  auto const trajectory_in_plotting_frame =
      RenderBarycentricTrajectoryInPlotting(begin, end);
  auto trajectory_in_world =
      RenderPlottingTrajectoryInWorld(time,
                                      trajectory_in_plotting_frame.begin(),
                                      trajectory_in_plotting_frame.end(),
                                      sun_world_position,
                                      planetarium_rotation);
  return trajectory_in_world;
}

DiscreteTrajectory<Navigation>
Renderer::RenderBarycentricTrajectoryInPlotting(
    DiscreteTrajectory<Barycentric>::iterator const& begin,
    DiscreteTrajectory<Barycentric>::iterator const& end) const {
  DiscreteTrajectory<Navigation> trajectory;
  for (auto it = begin; it != end; ++it) {
    auto const& [time, degrees_of_freedom] = *it;
    if (target_) {
      auto const prediction = target_->vessel->prediction();
      if (time < prediction->t_min()) {
        continue;
      } else if (time > prediction->t_max()) {
        break;
      }
    }
    trajectory.Append(time,
                      BarycentricToPlotting(time)(degrees_of_freedom))
        .IgnoreError();
  }
  return trajectory;
}

DiscreteTrajectory<World>
Renderer::RenderPlottingTrajectoryInWorld(
    Instant const& time,
    DiscreteTrajectory<Navigation>::iterator const& begin,
    DiscreteTrajectory<Navigation>::iterator const& end,
    Position<World> const& sun_world_position,
    Rotation<Barycentric, AliceSun> const& planetarium_rotation) const {
  DiscreteTrajectory<World> trajectory;

  //   Dinanzi a me non fuor cose create
  //   se non etterne, e io etterno duro.
  //   Lasciate ogne speranza, voi ch’intrate.
  //
  // This function does unnatural things.
  // - It identifies positions in the plotting frame with those of world using
  // the rigid transformation at the current time, instead of transforming each
  // position according to the transformation at its time.  This hides the fact
  // that we are considering an observer fixed in the plotting frame.
  // - Instead of applying the full rigid motion and consistently transforming
  // the velocities, or even just applying the orthogonal map, it simply
  // identifies the coordinates of |World| with those of the plotting frame.
  // This is because we are interested in the magnitude of the velocity (the
  // speed) in the plotting frame, as well as the coordinates (in frames with a
  // physically significant plane, the z coordinate becomes the out-of-plane
  // velocity).
  // The resulting |DegreesOfFreedom| should be seen as no more than a
  // convenient hack to send a plottable position together with a velocity in
  // the coordinates we want.  In fact, it needs an articial permutation to
  // avoid a violation of handedness.
  // TODO(phl): This will no longer be needed once we have support for
  // projections; instead of these convenient lies we can simply say that the
  // camera is fixed in the plotting frame and project there; additional data
  // can be gathered from the velocities in the plotting frame as needed and
  // sent directly to be shown in markers.
  RigidTransformation<Navigation, World> const
      from_plotting_frame_to_world_at_current_time =
          PlottingToWorld(time, sun_world_position, planetarium_rotation);
  for (auto it = begin; it != end; ++it) {
    auto const& [time, degrees_of_freedom] = *it;
    DegreesOfFreedom<Navigation> const& navigation_degrees_of_freedom =
        degrees_of_freedom;
    DegreesOfFreedom<World> const world_degrees_of_freedom = {
        from_plotting_frame_to_world_at_current_time(
            navigation_degrees_of_freedom.position()),
        geometry::Permutation<Navigation, World>(
            geometry::Permutation<Navigation,
                                  World>::CoordinatePermutation::YXZ)(
            navigation_degrees_of_freedom.velocity())};
    trajectory.Append(time, world_degrees_of_freedom).IgnoreError();
  }
  return trajectory;
}

RigidMotion<Barycentric, Navigation> Renderer::BarycentricToPlotting(
    Instant const& time) const {
  return GetPlottingFrame()->ToThisFrameAtTime(time);
}

RigidTransformation<Barycentric, World> Renderer::BarycentricToWorld(
    Instant const& time,
    Position<World> const& sun_world_position,
    Rotation<Barycentric, AliceSun> const& planetarium_rotation) const {
  return RigidTransformation<Barycentric, World>(
      sun_->current_position(time),
      sun_world_position,
      BarycentricToWorld(planetarium_rotation));
}

OrthogonalMap<Barycentric, World> Renderer::BarycentricToWorld(
    Rotation<Barycentric, AliceSun> const& planetarium_rotation) const {
  return OrthogonalMap<WorldSun, World>::Identity() *
         BarycentricToWorldSun(planetarium_rotation);
}

OrthogonalMap<Barycentric, WorldSun> Renderer::BarycentricToWorldSun(
    Rotation<Barycentric, AliceSun> const& planetarium_rotation) const {
  return sun_looking_glass.Inverse().Forget<OrthogonalMap>() *
         planetarium_rotation.Forget<OrthogonalMap>();
}

OrthogonalMap<Frenet<Navigation>, World> Renderer::FrenetToWorld(
    Instant const& time,
    NavigationManœuvre const& manœuvre,
    Rotation<Barycentric, AliceSun> const& planetarium_rotation) const {
  Instant const& initial_time = manœuvre.initial_time();
  return PlottingToWorld(time, planetarium_rotation) *
         BarycentricToPlotting(initial_time).orthogonal_map() *
         manœuvre.FrenetFrame();
}

OrthogonalMap<Frenet<Navigation>, World> Renderer::FrenetToWorld(
    Vessel const& vessel,
    Rotation<Barycentric, AliceSun> const& planetarium_rotation) const {
  auto const back = vessel.psychohistory()->back();
  DegreesOfFreedom<Barycentric> const& barycentric_degrees_of_freedom =
      back.degrees_of_freedom;
  DegreesOfFreedom<Navigation> const plotting_frame_degrees_of_freedom =
      BarycentricToPlotting(back.time)(barycentric_degrees_of_freedom);
  Rotation<Frenet<Navigation>, Navigation> const
      frenet_frame_to_plotting_frame =
          GetPlottingFrame()->FrenetFrame(
              back.time,
              plotting_frame_degrees_of_freedom);

  return PlottingToWorld(back.time, planetarium_rotation) *
         frenet_frame_to_plotting_frame.Forget<OrthogonalMap>();
}

OrthogonalMap<Frenet<Navigation>, World> Renderer::FrenetToWorld(
    Vessel const& vessel,
    NavigationFrame const& navigation_frame,
    Rotation<Barycentric, AliceSun> const& planetarium_rotation) const {
  auto const back = vessel.psychohistory()->back();
  auto const to_navigation = navigation_frame.ToThisFrameAtTime(back.time);
  auto const from_navigation = to_navigation.orthogonal_map().Inverse();
  auto const frenet_frame =
      navigation_frame.FrenetFrame(
          back.time,
          to_navigation(back.degrees_of_freedom)).Forget<OrthogonalMap>();
  return BarycentricToWorld(planetarium_rotation) * from_navigation *
         frenet_frame;
}

OrthogonalMap<Navigation, Barycentric> Renderer::PlottingToBarycentric(
    Instant const& time) const {
  return GetPlottingFrame()->FromThisFrameAtTime(time).orthogonal_map();
}

RigidTransformation<Navigation, World> Renderer::PlottingToWorld(
    Instant const& time,
    Position<World> const& sun_world_position,
    Rotation<Barycentric, AliceSun> const& planetarium_rotation) const {
  return BarycentricToWorld(time, sun_world_position, planetarium_rotation) *
         GetPlottingFrame()-> FromThisFrameAtTime(time).rigid_transformation();
}

OrthogonalMap<Navigation, World> Renderer::PlottingToWorld(
    Instant const& time,
    Rotation<Barycentric, AliceSun> const& planetarium_rotation) const {
  return BarycentricToWorld(planetarium_rotation) *
         PlottingToBarycentric(time);
}

RigidTransformation<World, Barycentric> Renderer::WorldToBarycentric(
    Instant const& time,
    Position<World> const& sun_world_position,
    Rotation<Barycentric, AliceSun> const& planetarium_rotation) const {
  return BarycentricToWorld(time, sun_world_position, planetarium_rotation)
      .Inverse();
}

OrthogonalMap<World, Barycentric> Renderer::WorldToBarycentric(
    Rotation<Barycentric, AliceSun> const& planetarium_rotation) const {
  return BarycentricToWorld(planetarium_rotation).Inverse();
}

RigidTransformation<World, Navigation> Renderer::WorldToPlotting(
    Instant const& time,
    Position<World> const& sun_world_position,
    Rotation<Barycentric, AliceSun> const& planetarium_rotation) const {
  return BarycentricToPlotting(time).rigid_transformation() *
         WorldToBarycentric(time, sun_world_position, planetarium_rotation);
}

Rotation<CameraCompensatedReference, World> Renderer::CameraReferenceRotation(
    Instant const& time,
    Rotation<Barycentric, AliceSun> const& planetarium_rotation,
    Rotation<CameraCompensatedReference, CameraReference> const&
        camera_compensation) const {
  Permutation<Barycentric, CelestialSphere> const celestial_mirror(
      OddPermutation::XZY);
  Permutation<CameraReference, Navigation> const camera_mirror(
      OddPermutation::XZY);
  return (BarycentricToWorld(planetarium_rotation) *
          GetPlottingFrame()->FromThisFrameAtTime(time).orthogonal_map() *
          camera_mirror.Forget<OrthogonalMap>()).AsRotation() *
         camera_compensation;
}

void Renderer::WriteToMessage(
    not_null<serialization::Renderer*> message) const {
  plotting_frame_->WriteToMessage(message->mutable_plotting_frame());
  // No serialization of the |target_|.
}

not_null<std::unique_ptr<Renderer>> Renderer::ReadFromMessage(
    serialization::Renderer const& message,
    not_null<Celestial const*> sun,
    not_null<Ephemeris<Barycentric> const*> const ephemeris) {
  return make_not_null_unique<Renderer>(
      sun,
      NavigationFrame::ReadFromMessage(message.plotting_frame(), ephemeris));
}

Renderer::Target::Target(
    not_null<Vessel*> const vessel,
    not_null<Celestial const*> const celestial,
    not_null<Ephemeris<Barycentric> const*> const ephemeris)
    : vessel(vessel),
      celestial(celestial),
      target_frame(
          make_not_null_unique<
              BodyCentredBodyDirectionDynamicFrame<Barycentric, Navigation>>(
              ephemeris,
              [this]() -> auto& { return *this->vessel->prediction(); },
              celestial->body())) {}

}  // namespace internal_renderer
}  // namespace ksp_plugin
}  // namespace principia
