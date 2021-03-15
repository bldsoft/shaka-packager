// Copyright 2020 Google LLC. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef PACKAGER_MEDIA_DVB_DVB_TELETEXT_H_
#define PACKAGER_MEDIA_DVB_DVB_TELETEXT_H_

#include <cstdint>

namespace shaka {
namespace media {

//!
//! Size in bytes of a Teletext packet.
//!
constexpr std::size_t TELETEXT_PACKET_SIZE = 44;

//!
//! First EBU data_identifier value in PES packets conveying Teletext.
//!
constexpr uint8_t TELETEXT_PES_FIRST_EBU_DATA_ID = 0x10;

//!
//! Last EBU data_identifier value in PES packets conveying Teletext.
//!
constexpr uint8_t TELETEXT_PES_LAST_EBU_DATA_ID = 0x1F;

//!
//! Teletext data unit ids.
//! @see ETSI EN 300 472
//!
enum class TeletextDataUnitId : uint8_t {
  NON_SUBTITLE = 0x02,  //!< Data_unit_id for EBU Teletext non-subtitle data.
  SUBTITLE = 0x03,      //!< Data_unit_id for EBU Teletext subtitle data.
  INVERTED =
      0x0C,    //!< Data_unit_id for EBU EBU Teletext Inverted (extension ?).
  VPS = 0xC3,  //!< Data_unit_id for VPS (extension ?).
  CLOSED_CAPTIONS = 0xC5,  //!< Data_unit_id for Closed Caption (extension ?).
  STUFFING = 0xFF,         //!< Data_unit_id for stuffing data.
};
}
}

#endif