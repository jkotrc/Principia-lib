﻿
#pragma once

#include "physics/rigid_motion.hpp"

#include <utility>

#include "geometry/identity.hpp"
#include "geometry/linear_map.hpp"

namespace principia {
namespace physics {
namespace internal_rigid_motion {

using geometry::LinearMap;

template<typename FromFrame, typename ToFrame>
RigidMotion<FromFrame, ToFrame>::RigidMotion(
    RigidTransformation<FromFrame, ToFrame> const& rigid_transformation,
    AngularVelocity<FromFrame> const& angular_velocity_of_to_frame,
    Velocity<FromFrame> const& velocity_of_to_frame_origin)
    : rigid_transformation_(rigid_transformation),
      angular_velocity_of_to_frame_(angular_velocity_of_to_frame),
      velocity_of_to_frame_origin_(velocity_of_to_frame_origin) {}

template<typename FromFrame, typename ToFrame>
template<typename F, typename T, typename>
RigidMotion<FromFrame, ToFrame>::RigidMotion(
    RigidTransformation<FromFrame, ToFrame> const& rigid_transformation,
    AngularVelocity<ToFrame> const& angular_velocity_of_from_frame,
    Velocity<ToFrame> const& velocity_of_from_frame_origin)
    : RigidMotion(
          RigidMotion<ToFrame, FromFrame>(rigid_transformation.Inverse(),
                                          angular_velocity_of_from_frame,
                                          velocity_of_from_frame_origin)
              .Inverse()) {}

template<typename FromFrame, typename ToFrame>
RigidTransformation<FromFrame, ToFrame> const&
RigidMotion<FromFrame, ToFrame>::rigid_transformation() const {
  return rigid_transformation_;
}

template<typename FromFrame, typename ToFrame>
OrthogonalMap<FromFrame, ToFrame> const&
RigidMotion<FromFrame, ToFrame>::orthogonal_map() const {
  return rigid_transformation_.linear_map();
}

template<typename FromFrame, typename ToFrame>
template<typename F>
AngularVelocity<
    typename RigidMotion<FromFrame, ToFrame>::template other_frame_t<F>>
RigidMotion<FromFrame, ToFrame>::angular_velocity_of() const {
  if constexpr (std::is_same_v<F, ToFrame>) {
    return angular_velocity_of_to_frame_;
  } else if constexpr (std::is_same_v<F, FromFrame>) {
    return Inverse().angular_velocity_of_to_frame_;
  } else {
    static_assert(std::is_same_v<F, ToFrame> || std::is_same_v<F, FromFrame>,
                  "Nonsensical frame");
  }
}

template<typename FromFrame, typename ToFrame>
template<typename F>
Velocity<typename RigidMotion<FromFrame, ToFrame>::template other_frame_t<F>>
RigidMotion<FromFrame, ToFrame>::velocity_of_origin_of() const {
  if constexpr (std::is_same_v<F, ToFrame>) {
    return velocity_of_to_frame_origin_;
  } else if constexpr (std::is_same_v<F, FromFrame>) {
    return Inverse().velocity_of_to_frame_origin_;
  } else {
    static_assert(std::is_same_v<F, ToFrame> || std::is_same_v<F, FromFrame>,
                  "Nonsensical frame");
  }
}

template<typename FromFrame, typename ToFrame>
DegreesOfFreedom<ToFrame> RigidMotion<FromFrame, ToFrame>::operator()(
    DegreesOfFreedom<FromFrame> const& degrees_of_freedom) const {
  return {rigid_transformation_(degrees_of_freedom.position()),
          orthogonal_map()(
              degrees_of_freedom.velocity() - velocity_of_to_frame_origin_ -
              angular_velocity_of_to_frame_ *
                  (degrees_of_freedom.position() -
                   rigid_transformation_.Inverse()(ToFrame::origin)) /
                  Radian)};
}

template<typename FromFrame, typename ToFrame>
RigidMotion<ToFrame, FromFrame>
RigidMotion<FromFrame, ToFrame>::Inverse() const {
  return RigidMotion<ToFrame, FromFrame>(
      rigid_transformation_.Inverse(),
      -orthogonal_map()(angular_velocity_of_to_frame_),
      (*this)({FromFrame::origin, FromFrame::unmoving}).velocity());
}

template<typename FromFrame, typename ToFrame>
RigidMotion<FromFrame, ToFrame>
RigidMotion<FromFrame, ToFrame>::MakeNonRotatingMotion(
    DegreesOfFreedom<ToFrame> const& degrees_of_freedom_of_from_frame_origin) {
  if constexpr (FromFrame::handedness == ToFrame::handedness) {
    return RigidMotion(RigidTransformation<FromFrame, ToFrame>(
                           FromFrame::origin,
                           degrees_of_freedom_of_from_frame_origin.position(),
                           geometry::Identity<FromFrame, ToFrame>().Forget()),
                       ToFrame::nonrotating,
                       degrees_of_freedom_of_from_frame_origin.velocity());
  } else {
    // TODO(phl): This is extremely dubious.  We apply the sun_looking_glass
    // permutation because we "know" that we have World and RigidPart here.
    return RigidMotion(
        RigidTransformation<FromFrame, ToFrame>(
            FromFrame::origin,
            degrees_of_freedom_of_from_frame_origin.position(),
            geometry::Permutation<FromFrame, ToFrame>(
                geometry::Permutation<FromFrame,
                                      ToFrame>::CoordinatePermutation::XZY)
                .template Forget<OrthogonalMap>()),
        ToFrame::nonrotating,
        degrees_of_freedom_of_from_frame_origin.velocity());
  }
}

template<typename FromFrame, typename ToFrame>
void RigidMotion<FromFrame, ToFrame>::WriteToMessage(
    not_null<serialization::RigidMotion*> const message) const {
  rigid_transformation_.WriteToMessage(message->mutable_rigid_transformation());
  angular_velocity_of_to_frame_.WriteToMessage(
      message->mutable_angular_velocity_of_to_frame());
  velocity_of_to_frame_origin_.WriteToMessage(
      message->mutable_velocity_of_to_frame_origin());
}

template<typename FromFrame, typename ToFrame>
RigidMotion<FromFrame, ToFrame>
RigidMotion<FromFrame, ToFrame>::ReadFromMessage(
    serialization::RigidMotion const& message) {
  return RigidMotion(RigidTransformation<FromFrame, ToFrame>::ReadFromMessage(
                         message.rigid_transformation()),
                     AngularVelocity<FromFrame>::ReadFromMessage(
                         message.angular_velocity_of_to_frame()),
                     Velocity<FromFrame>::ReadFromMessage(
                         message.velocity_of_to_frame_origin()));
}

template<typename FromFrame, typename ToFrame>
template<typename F, typename T, typename>
RigidMotion<FromFrame, ToFrame> RigidMotion<FromFrame, ToFrame>::Identity() {
  return RigidMotion(RigidTransformation<FromFrame, ToFrame>::Identity(),
                     FromFrame::nonrotating,
                     FromFrame::unmoving);
}

template<typename FromFrame, typename ThroughFrame, typename ToFrame>
RigidMotion<FromFrame, ToFrame> operator*(
    RigidMotion<ThroughFrame, ToFrame> const& left,
    RigidMotion<FromFrame, ThroughFrame> const& right) {
  return RigidMotion<FromFrame, ToFrame>(
      left.rigid_transformation() * right.rigid_transformation(),
      right.angular_velocity_of_to_frame_ +
          right.orthogonal_map().Inverse()(left.angular_velocity_of_to_frame_),
      right.Inverse()(left.Inverse()(
          {ToFrame::origin, ToFrame::unmoving})).velocity());
}

template<typename FromFrame, typename ToFrame>
std::ostream& operator<<(std::ostream& out,
                         RigidMotion<FromFrame, ToFrame> const& rigid_motion) {
  return out << "{transformation: " << rigid_motion.rigid_transformation()
             << ", angular velocity: "
             << rigid_motion.template angular_velocity_of<ToFrame>()
             << ", velocity: "
             << rigid_motion.template velocity_of_origin_of<ToFrame>()
             << "}";
}

template<typename FromFrame, typename ToFrame>
std::ostream& operator<<(std::ostream& out,
                         AcceleratedRigidMotion<FromFrame, ToFrame> const&
                             accelerated_rigid_motion) {
  return out
         << "{motion: " << accelerated_rigid_motion.rigid_motion()
         << ", angular acceleration: "
         << accelerated_rigid_motion.template angular_acceleration_of<ToFrame>()
         << ", acceleration: "
         << accelerated_rigid_motion
                .template acceleration_of_origin_of<ToFrame>()
         << "}";
}

template<typename FromFrame, typename ToFrame>
AcceleratedRigidMotion<FromFrame, ToFrame>::AcceleratedRigidMotion(
    RigidMotion<FromFrame, ToFrame> const& rigid_motion,
    Bivector<AngularAcceleration, FromFrame> const&
        angular_acceleration_of_to_frame,
    Vector<Acceleration, FromFrame> const& acceleration_of_to_frame_origin)
    : rigid_motion_(rigid_motion),
      angular_acceleration_of_to_frame_(angular_acceleration_of_to_frame),
      acceleration_of_to_frame_origin_(acceleration_of_to_frame_origin) {}

template<typename FromFrame, typename ToFrame>
RigidMotion<FromFrame, ToFrame> const&
AcceleratedRigidMotion<FromFrame, ToFrame>::rigid_motion() const {
  return rigid_motion_;
}

template<typename FromFrame, typename ToFrame>
template<typename F>
Bivector<AngularAcceleration,
         typename AcceleratedRigidMotion<FromFrame, ToFrame>::
             template other_frame_t<F>>
AcceleratedRigidMotion<FromFrame, ToFrame>::angular_acceleration_of() const {
  if constexpr (std::is_same_v<F, ToFrame>) {
    return angular_acceleration_of_to_frame_;
  } else if constexpr (std::is_same_v<F, FromFrame>) {
    static_assert(!std::is_same_v<F, FromFrame>,
                  "Time to implement AcceleratedRigidMotion::Inverse");
  } else {
    static_assert(std::is_same_v<F, ToFrame> || std::is_same_v<F, FromFrame>,
                  "Nonsensical frame");
  }
}

template<typename FromFrame, typename ToFrame>
template<typename F>
Vector<Acceleration,
       typename AcceleratedRigidMotion<FromFrame, ToFrame>::
           template other_frame_t<F>>
AcceleratedRigidMotion<FromFrame, ToFrame>::acceleration_of_origin_of() const {
  if constexpr (std::is_same_v<F, ToFrame>) {
    return acceleration_of_to_frame_origin_;
  } else if constexpr (std::is_same_v<F, FromFrame>) {
    static_assert(!std::is_same_v<F, FromFrame>,
                  "Time to implement AcceleratedRigidMotion::Inverse");
  } else {
    static_assert(std::is_same_v<F, ToFrame> || std::is_same_v<F, FromFrame>,
                  "Nonsensical frame");
  }
}

}  // namespace internal_rigid_motion
}  // namespace physics
}  // namespace principia
