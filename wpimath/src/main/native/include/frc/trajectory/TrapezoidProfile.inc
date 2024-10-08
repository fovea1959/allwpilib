// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

#pragma once

#include <algorithm>

#include "frc/trajectory/TrapezoidProfile.h"
#include "units/math.h"

namespace frc {
template <class Distance>
TrapezoidProfile<Distance>::TrapezoidProfile(Constraints constraints)
    : m_constraints(constraints) {}

template <class Distance>
typename TrapezoidProfile<Distance>::State
TrapezoidProfile<Distance>::Calculate(units::second_t t, State current,
                                      State goal) {
  m_direction = ShouldFlipAcceleration(current, goal) ? -1 : 1;
  m_current = Direct(current);
  goal = Direct(goal);
  if (m_current.velocity > m_constraints.maxVelocity) {
    m_current.velocity = m_constraints.maxVelocity;
  }

  // Deal with a possibly truncated motion profile (with nonzero initial or
  // final velocity) by calculating the parameters as if the profile began and
  // ended at zero velocity
  units::second_t cutoffBegin =
      m_current.velocity / m_constraints.maxAcceleration;
  Distance_t cutoffDistBegin =
      cutoffBegin * cutoffBegin * m_constraints.maxAcceleration / 2.0;

  units::second_t cutoffEnd = goal.velocity / m_constraints.maxAcceleration;
  Distance_t cutoffDistEnd =
      cutoffEnd * cutoffEnd * m_constraints.maxAcceleration / 2.0;

  // Now we can calculate the parameters as if it was a full trapezoid instead
  // of a truncated one

  Distance_t fullTrapezoidDist =
      cutoffDistBegin + (goal.position - m_current.position) + cutoffDistEnd;
  units::second_t accelerationTime =
      m_constraints.maxVelocity / m_constraints.maxAcceleration;

  Distance_t fullSpeedDist =
      fullTrapezoidDist -
      accelerationTime * accelerationTime * m_constraints.maxAcceleration;

  // Handle the case where the profile never reaches full speed
  if (fullSpeedDist < Distance_t{0}) {
    accelerationTime =
        units::math::sqrt(fullTrapezoidDist / m_constraints.maxAcceleration);
    fullSpeedDist = Distance_t{0};
  }

  m_endAccel = accelerationTime - cutoffBegin;
  m_endFullSpeed = m_endAccel + fullSpeedDist / m_constraints.maxVelocity;
  m_endDecel = m_endFullSpeed + accelerationTime - cutoffEnd;
  State result = m_current;

  if (t < m_endAccel) {
    result.velocity += t * m_constraints.maxAcceleration;
    result.position +=
        (m_current.velocity + t * m_constraints.maxAcceleration / 2.0) * t;
  } else if (t < m_endFullSpeed) {
    result.velocity = m_constraints.maxVelocity;
    result.position += (m_current.velocity +
                        m_endAccel * m_constraints.maxAcceleration / 2.0) *
                           m_endAccel +
                       m_constraints.maxVelocity * (t - m_endAccel);
  } else if (t <= m_endDecel) {
    result.velocity =
        goal.velocity + (m_endDecel - t) * m_constraints.maxAcceleration;
    units::second_t timeLeft = m_endDecel - t;
    result.position =
        goal.position -
        (goal.velocity + timeLeft * m_constraints.maxAcceleration / 2.0) *
            timeLeft;
  } else {
    result = goal;
  }

  return Direct(result);
}

template <class Distance>
units::second_t TrapezoidProfile<Distance>::TimeLeftUntil(
    Distance_t target) const {
  Distance_t position = m_current.position * m_direction;
  Velocity_t velocity = m_current.velocity * m_direction;

  units::second_t endAccel = m_endAccel * m_direction;
  units::second_t endFullSpeed = m_endFullSpeed * m_direction - endAccel;

  if (target < position) {
    endAccel *= -1.0;
    endFullSpeed *= -1.0;
    velocity *= -1.0;
  }

  endAccel = units::math::max(endAccel, 0_s);
  endFullSpeed = units::math::max(endFullSpeed, 0_s);

  const Acceleration_t acceleration = m_constraints.maxAcceleration;
  const Acceleration_t deceleration = -m_constraints.maxAcceleration;

  Distance_t distToTarget = units::math::abs(target - position);

  if (distToTarget < Distance_t{1e-6}) {
    return 0_s;
  }

  Distance_t accelDist =
      velocity * endAccel + 0.5 * acceleration * endAccel * endAccel;

  Velocity_t decelVelocity;
  if (endAccel > 0_s) {
    decelVelocity = units::math::sqrt(
        units::math::abs(velocity * velocity + 2 * acceleration * accelDist));
  } else {
    decelVelocity = velocity;
  }

  Distance_t fullSpeedDist = m_constraints.maxVelocity * endFullSpeed;
  Distance_t decelDist;

  if (accelDist > distToTarget) {
    accelDist = distToTarget;
    fullSpeedDist = Distance_t{0};
    decelDist = Distance_t{0};
  } else if (accelDist + fullSpeedDist > distToTarget) {
    fullSpeedDist = distToTarget - accelDist;
    decelDist = Distance_t{0};
  } else {
    decelDist = distToTarget - fullSpeedDist - accelDist;
  }

  units::second_t accelTime =
      (-velocity + units::math::sqrt(units::math::abs(
                       velocity * velocity + 2 * acceleration * accelDist))) /
      acceleration;

  units::second_t decelTime =
      (-decelVelocity +
       units::math::sqrt(units::math::abs(decelVelocity * decelVelocity +
                                          2 * deceleration * decelDist))) /
      deceleration;

  units::second_t fullSpeedTime = fullSpeedDist / m_constraints.maxVelocity;

  return accelTime + fullSpeedTime + decelTime;
}
}  // namespace frc
