// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

#pragma once

#include <stdint.h>

#include <array>

#include <wpi/mutex.h>
#include <wpi/sendable/Sendable.h>
#include <wpi/sendable/SendableHelper.h>

#include "frc/DigitalSource.h"

namespace frc {

class Encoder;
class Counter;

/**
 * Class to enable glitch filtering on a set of digital inputs.
 *
 * This class will manage adding and removing digital inputs from a FPGA glitch
 * filter. The filter lets the user configure the time that an input must remain
 * high or low before it is classified as high or low.
 */
class DigitalGlitchFilter : public wpi::Sendable,
                            public wpi::SendableHelper<DigitalGlitchFilter> {
 public:
  DigitalGlitchFilter();
  ~DigitalGlitchFilter() override;

  DigitalGlitchFilter(DigitalGlitchFilter&&) = default;
  DigitalGlitchFilter& operator=(DigitalGlitchFilter&&) = default;

  /**
   * Assigns the DigitalSource to this glitch filter.
   *
   * @param input The DigitalSource to add.
   */
  void Add(DigitalSource* input);

  /**
   * Assigns the Encoder to this glitch filter.
   *
   * @param input The Encoder to add.
   */
  void Add(Encoder* input);

  /**
   * Assigns the Counter to this glitch filter.
   *
   * @param input The Counter to add.
   */
  void Add(Counter* input);

  /**
   * Removes a digital input from this filter.
   *
   * Removes the DigitalSource from this glitch filter and re-assigns it to
   * the default filter.
   *
   * @param input The DigitalSource to remove.
   */
  void Remove(DigitalSource* input);

  /**
   * Removes an encoder from this filter.
   *
   * Removes the Encoder from this glitch filter and re-assigns it to
   * the default filter.
   *
   * @param input The Encoder to remove.
   */
  void Remove(Encoder* input);

  /**
   * Removes a counter from this filter.
   *
   * Removes the Counter from this glitch filter and re-assigns it to
   * the default filter.
   *
   * @param input The Counter to remove.
   */
  void Remove(Counter* input);

  /**
   * Sets the number of cycles that the input must not change state for.
   *
   * @param fpgaCycles The number of FPGA cycles.
   */
  void SetPeriodCycles(int fpgaCycles);

  /**
   * Sets the number of nanoseconds that the input must not change state for.
   *
   * @param nanoseconds The number of nanoseconds.
   */
  void SetPeriodNanoSeconds(uint64_t nanoseconds);

  /**
   * Gets the number of cycles that the input must not change state for.
   *
   * @return The number of cycles.
   */
  int GetPeriodCycles();

  /**
   * Gets the number of nanoseconds that the input must not change state for.
   *
   * @return The number of nanoseconds.
   */
  uint64_t GetPeriodNanoSeconds();

  void InitSendable(wpi::SendableBuilder& builder) override;

 private:
  int m_channelIndex;

  // Sets the filter for the input to be the requested index. A value of 0
  // disables the filter, and the filter value must be between 1 and 3,
  // inclusive.
  static void DoAdd(DigitalSource* input, int requested_index);

  /**
   * Allocates the next available filter index, or -1 if there are no filters
   * available.
   * @return the filter index
   */
  static int AllocateFilterIndex();

  static wpi::mutex m_mutex;
  static std::array<bool, 3> m_filterAllocated;
};

}  // namespace frc
