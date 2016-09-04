﻿
#pragma once

#include "physics/body_surface_dynamic_frame.hpp"

#include "geometry/rotation.hpp"

namespace principia {
namespace physics {
namespace internal_body_surface_dynamic_frame {

using geometry::AngularVelocity;
using geometry::Rotation;

template<typename InertialFrame, typename ThisFrame>
BodySurfaceDynamicFrame<InertialFrame, ThisFrame>::
BodySurfaceDynamicFrame(
    not_null<Ephemeris<InertialFrame> const*> const ephemeris,
    not_null<RotatingBody<InertialFrame> const*> const centre)
    : ephemeris_(ephemeris),
      centre_(centre),
      centre_trajectory_(ephemeris_->trajectory(centre_)) {}

template<typename InertialFrame, typename ThisFrame>
RigidMotion<InertialFrame, ThisFrame>
BodySurfaceDynamicFrame<InertialFrame, ThisFrame>::ToThisFrameAtTime(
    Instant const& t) const {
  DegreesOfFreedom<InertialFrame> const centre_degrees_of_freedom =
      centre_trajectory_->EvaluateDegreesOfFreedom(t, &hint_);

  Rotation<InertialFrame, ThisFrame> rotation = centre_->ToSurfaceFrame(t);
  AngularVelocity<InertialFrame> angular_velocity = centre_->angular_velocity();
  RigidTransformation<InertialFrame, ThisFrame> const
      rigid_transformation(centre_degrees_of_freedom.position(),
                           ThisFrame::origin,
                           rotation.Forget());
  return RigidMotion<InertialFrame, ThisFrame>(
             rigid_transformation,
             angular_velocity,
             centre_degrees_of_freedom.velocity());
}

template<typename InertialFrame, typename ThisFrame>
void BodySurfaceDynamicFrame<InertialFrame, ThisFrame>::
WriteToMessage(not_null<serialization::DynamicFrame*> const message) const {
  message->MutableExtension(
      serialization::BodySurfaceDynamicFrame::extension)->set_centre(
          ephemeris_->serialization_index_for_body(centre_));
}

template<typename InertialFrame, typename ThisFrame>
not_null<std::unique_ptr<
    BodySurfaceDynamicFrame<InertialFrame, ThisFrame>>>
BodySurfaceDynamicFrame<InertialFrame, ThisFrame>::ReadFromMessage(
    not_null<Ephemeris<InertialFrame> const*> const ephemeris,
    serialization::BodySurfaceDynamicFrame const& message) {
  return std::make_unique<BodySurfaceDynamicFrame>(
             ephemeris,
             ephemeris->body_for_serialization_index(message.centre()));
}

template<typename InertialFrame, typename ThisFrame>
Vector<Acceleration, InertialFrame>
BodySurfaceDynamicFrame<InertialFrame, ThisFrame>::
    GravitationalAcceleration(Instant const& t,
                              Position<InertialFrame> const& q) const {
  return ephemeris_->ComputeGravitationalAccelerationOnMasslessBody(q, t);
}

template<typename InertialFrame, typename ThisFrame>
AcceleratedRigidMotion<InertialFrame, ThisFrame>
BodySurfaceDynamicFrame<InertialFrame, ThisFrame>::MotionOfThisFrame(
    Instant const& t) const {
  DegreesOfFreedom<InertialFrame> const centre_degrees_of_freedom =
      centre_trajectory_->EvaluateDegreesOfFreedom(t, &primary_hint_);
  Vector<Acceleration, InertialFrame> const centre_acceleration =
      ephemeris_->ComputeGravitationalAccelerationOnMassiveBody(centre_, t);

  auto const to_this_frame = ToThisFrameAtTime(t);

  Variation<AngularVelocity<InertialFrame>> const
      angular_acceleration_of_to_frame;
  Vector<Acceleration, InertialFrame> const& acceleration_of_to_frame_origin =
      centre_acceleration;
  return AcceleratedRigidMotion<InertialFrame, ThisFrame>(
             to_this_frame,
             angular_acceleration_of_to_frame,
             acceleration_of_to_frame_origin);
}

}  // namespace internal_body_surface_dynamic_frame
}  // namespace physics
}  // namespace principia
