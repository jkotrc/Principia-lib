﻿
#pragma once

#include <functional>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <vector>

#include "absl/status/status.h"
#include "base/not_constructible.hpp"
#include "base/not_null.hpp"
#include "geometry/grassmann.hpp"
#include "geometry/named_quantities.hpp"
#include "numerics/hermite3.hpp"
#include "physics/degrees_of_freedom.hpp"
#include "physics/forkable.hpp"
#include "physics/trajectory.hpp"
#include "quantities/named_quantities.hpp"
#include "serialization/physics.pb.h"

namespace principia {
namespace physics {

FORWARD_DECLARE_FROM(discrete_trajectory,
                     TEMPLATE(typename Frame) class,
                     DiscreteTrajectory);

// Reopening |internal_forkable| to specialize a template.
namespace internal_forkable {

using base::not_constructible;

template<typename Frame>
struct DiscreteTrajectoryTraits : not_constructible {
  using Timeline = typename std::map<Instant, DegreesOfFreedom<Frame>>;
  using TimelineConstIterator = typename Timeline::const_iterator;

  static Instant const& time(TimelineConstIterator it);
};

template<typename Frame>
class DiscreteTrajectoryIterator
    : public ForkableIterator<DiscreteTrajectory<Frame>,
                              DiscreteTrajectoryIterator<Frame>,
                              DiscreteTrajectoryTraits<Frame>> {
 public:
  struct reference {
    explicit reference(
        typename DiscreteTrajectoryTraits<Frame>::TimelineConstIterator it);

    Instant const& time;
    DegreesOfFreedom<Frame> const& degrees_of_freedom;
  };

  reference operator*() const;
  std::optional<reference> operator->() const;

 protected:
  not_null<DiscreteTrajectoryIterator*> that() override;
  not_null<DiscreteTrajectoryIterator const*> that() const override;
};

}  // namespace internal_forkable

namespace internal_discrete_trajectory {

using base::not_null;
using geometry::Instant;
using geometry::Position;
using geometry::Vector;
using geometry::Velocity;
using internal_forkable::DiscreteTrajectoryIterator;
using internal_forkable::DiscreteTrajectoryTraits;
using quantities::Acceleration;
using quantities::Length;
using quantities::Speed;
using numerics::Hermite3;

template<typename Frame>
class DiscreteTrajectory : public Forkable<DiscreteTrajectory<Frame>,
                                           DiscreteTrajectoryIterator<Frame>,
                                           DiscreteTrajectoryTraits<Frame>>,
                           public Trajectory<Frame> {
 public:
  using Iterator = DiscreteTrajectoryIterator<Frame>;

  DiscreteTrajectory() = default;
  DiscreteTrajectory(DiscreteTrajectory const&) = delete;
  DiscreteTrajectory(DiscreteTrajectory&&) = delete;
  DiscreteTrajectory& operator=(DiscreteTrajectory const&) = delete;
  DiscreteTrajectory& operator=(DiscreteTrajectory&&) = delete;

  // Creates a new child trajectory forked at time |time|, and returns it.  The
  // child trajectory shares its data with the current trajectory for times less
  // than or equal to |time|, and is an exact copy of the current trajectory for
  // times greater than |time|.  It may be changed independently from the
  // parent trajectory for any time (strictly) greater than |time|.  The child
  // trajectory is owned by its parent trajectory.  Deleting the parent
  // trajectory deletes all child trajectories.  |time| must be one of the times
  // of this trajectory, and must be at or after the fork time, if any.
  not_null<DiscreteTrajectory<Frame>*> NewForkWithCopy(Instant const& time);

  // Same as above, except that the parent trajectory after the fork point is
  // not copied.
  not_null<DiscreteTrajectory<Frame>*> NewForkWithoutCopy(Instant const& time);

  // Same as above, except that the fork is created at the last point of the
  // trajectory.
  not_null<DiscreteTrajectory<Frame>*> NewForkAtLast();

  // Changes |fork| to become a fork of this trajectory at the end of this
  // trajectory.  |fork| must be a non-empty root and must start at or after the
  // last time of this trajectory.  If it has a point at the last time of this
  // trajectory, that point is ignored.
  void AttachFork(not_null<std::unique_ptr<DiscreteTrajectory<Frame>>> fork);

  // This object must not be a root.  It is detached from its parent and becomes
  // a root.  A point corresponding to the fork point is prepended to this
  // object (so it's never empty) and an owning pointer to it is returned.
  not_null<std::unique_ptr<DiscreteTrajectory<Frame>>> DetachFork();

  // Appends one point to the trajectory.
  absl::Status Append(Instant const& time,
                      DegreesOfFreedom<Frame> const& degrees_of_freedom);

  // Removes all data for times (strictly) greater than |time|, as well as all
  // child trajectories forked at times (strictly) greater than |time|.  |time|
  // must be at or after the fork time, if any.
  void ForgetAfter(Instant const& time);

  // Removes all data for times (strictly) less than |time|, and checks that
  // there are no child trajectories forked at times (strictly) less than
  // |time|.  This trajectory must be a root.
  void ForgetBefore(Instant const& time);

  // This trajectory must be root, and must not be already downsampling.
  // Following this call, this trajectory must not have forks when calling
  // |Append|.  Occasionally removes intermediate points from the trajectory
  // when |Append|ing, ensuring that |EvaluatePosition| returns a result within
  // |tolerance| of the missing points.  |max_dense_intervals| is the largest
  // number of points that can be added before removal is considered.
  void SetDownsampling(std::int64_t max_dense_intervals,
                       Length const& tolerance);

  // Clear the downsampling parameters.  From now on, all points appended to the
  // trajectory are going to be retained.
  void ClearDownsampling();

  // Implementation of the interface |Trajectory|.

  // The bounds are the times of |begin()| and |rbegin()| if this trajectory is
  // nonempty, otherwise they are infinities of the appropriate signs.
  Instant t_min() const override;
  Instant t_max() const override;

  Position<Frame> EvaluatePosition(Instant const& time) const override;
  Velocity<Frame> EvaluateVelocity(Instant const& time) const override;
  DegreesOfFreedom<Frame> EvaluateDegreesOfFreedom(
      Instant const& time) const override;

  // End of the implementation of the interface.

  // This trajectory must be a root.  The entire tree is traversed and the forks
  // not present in |excluded| serialized.  The forks in |tracked| will be
  // retrieved in the same order when reading.  The pointers may be null at
  // entry; otherwise, they must be direct or indirect forks of this trajectory.
  void WriteToMessage(not_null<serialization::DiscreteTrajectory*> message,
                      std::set<DiscreteTrajectory*> const& excluded,
                      std::vector<DiscreteTrajectory*> const& tracked) const;

  // |forks| must have a size appropriate for the |message| being deserialized
  // and the orders of the |forks| must be consistent during serialization and
  // deserialization.  All pointers designated by the pointers in |forks| must
  // be null at entry; they may be null at exit.
  template<typename F = Frame,
           typename = std::enable_if_t<base::is_serializable_v<F>>>
  static not_null<std::unique_ptr<DiscreteTrajectory>> ReadFromMessage(
      serialization::DiscreteTrajectory const& message,
      std::vector<DiscreteTrajectory<Frame>**> const& tracked);

 protected:
  using TimelineConstIterator =
      typename DiscreteTrajectoryTraits<Frame>::TimelineConstIterator;

  // The API inherited from Forkable.
  not_null<DiscreteTrajectory*> that() override;
  not_null<DiscreteTrajectory const*> that() const override;

  TimelineConstIterator timeline_begin() const override;
  TimelineConstIterator timeline_end() const override;
  TimelineConstIterator timeline_find(Instant const& time) const override;
  TimelineConstIterator timeline_lower_bound(
                            Instant const& time) const override;
  bool timeline_empty() const override;
  std::int64_t timeline_size() const override;

 private:
  using Timeline = typename DiscreteTrajectoryTraits<Frame>::Timeline;

  // A helper class to manage a dense timeline.
  class Downsampling {
   public:
    Downsampling(std::int64_t max_dense_intervals,
                 Length tolerance);

    // Construction parameters.
    std::int64_t max_dense_intervals() const;
    Length tolerance() const;

    // Appends a point to the dense timeline.
    void Append(TimelineConstIterator it);

    // Forgets the points of the dense timeline after/before t.  The semantics
    // are the same as that of the corresponding functions of
    // DiscreteTrajectory.
    void ForgetAfter(Instant const& t);
    void ForgetBefore(Instant const& t);

    bool empty() const;
    bool full() const;

    // Returns the |dense_iterators_|, giving ownership to the caller.
    std::vector<TimelineConstIterator> dense_iterators();

    void WriteToMessage(
        not_null<serialization::DiscreteTrajectory::Downsampling*> message)
        const;
    static Downsampling ReadFromMessage(
        serialization::DiscreteTrajectory::Downsampling const& message,
        Timeline const& timeline);

   private:
    // The maximal number of dense intervals before downsampling occurs.
    std::int64_t const max_dense_intervals_;
    // The tolerance for downsampling with |FitHermiteSpline|.
    Length const tolerance_;

    // TODO(phl): Note that, with forks, the iterators in this vector may belong
    // to different maps.
    std::vector<TimelineConstIterator> dense_iterators_;
  };

  // This trajectory need not be a root.
  void WriteSubTreeToMessage(
      not_null<serialization::DiscreteTrajectory*> message,
      std::set<DiscreteTrajectory*>& excluded,
      std::vector<DiscreteTrajectory*>& tracked) const;

  void FillSubTreeFromMessage(
      serialization::DiscreteTrajectory const& message,
      std::vector<DiscreteTrajectory<Frame>**> const& tracked);

  // Returns the Hermite interpolation for the left-open, right-closed
  // trajectory segment bounded above by |upper|.
  Hermite3<Instant, Position<Frame>> GetInterpolation(
      Iterator const& upper) const;

  // Updates the downsampling object to reflect that a point was appended to
  // this trajectory.
  absl::Status UpdateDownsampling(TimelineConstIterator appended);

  Timeline timeline_;

  std::optional<Downsampling> downsampling_;

  template<typename, typename, typename>
  friend class internal_forkable::ForkableIterator;
  template<typename, typename, typename>
  friend class internal_forkable::Forkable;

  // For using the private constructor in maps.
  template<typename, typename>
  friend struct ::std::pair;
};

}  // namespace internal_discrete_trajectory

using internal_discrete_trajectory::DiscreteTrajectory;

}  // namespace physics
}  // namespace principia

#include "physics/discrete_trajectory_body.hpp"
