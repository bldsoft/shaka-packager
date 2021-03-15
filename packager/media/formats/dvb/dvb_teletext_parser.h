// Copyright 2020 Google LLC. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef PACKAGER_MEDIA_DVB_DVB_TELETEXT_PARSER_H_
#define PACKAGER_MEDIA_DVB_DVB_TELETEXT_PARSER_H_

#include "packager/media/base/bit_reader.h"
#include "packager/media/base/text_sample.h"
#include "packager/media/formats/dvb/dvb_teletext_charset.h"
#include "packager/media/formats/dvb/dvb_teletext.h"

#include <memory>
#include <unordered_map>

namespace shaka {
namespace media {

class DvbTeletextParser {
 public:
  //!
  //! Teletext transmission mode.
  //! Don't change values, they must match the binary format.
  //!
  enum TransMode {
    TRANSMODE_PARALLEL = 0,  //!< Parallel mode.
    TRANSMODE_SERIAL = 1     //!< Serial mode.
  };

  //!
  //! Structure of a Teletext page.
  //!
  class TeletextPage {
   public:
    enum PageSize : std::size_t { kRowsCount = 25, kColumnsCount = 40 };

   public:
    uint32_t frameCount = {};    //!< Number of produced frames in this page.
    int64_t showTimestamp = {};  //!< Show at timestamp (in ms).
    int64_t hideTimestamp = {};  //!< Hide at timestamp (in ms).
    bool tainted = {};           //!< True if text variable contains any data.
    DvbTeletextCharset charset;  //!< Charset to use.
    char16_t text[kRowsCount][kColumnsCount] =
        {};  //!< 25 lines x 40 cols (1 screen/page) of wide chars.
    //!
    //! Default constructor.
    //!
    TeletextPage() = default;
    //!
    //! Reset to a new page with a new starting timestamp.
    //! @param [in] timestamp New starting timestamp.
    //!
    void reset(int64_t timestamp) noexcept;
    //!
    //! Detect page is empty.
    //!
    bool is_empty() noexcept;
  };

  //!
  //! Map of TeletextPage, indexed by page number.
  //!
  using TeletextPageMap = std::unordered_map<uint16_t, TeletextPage>;

  //!
  //! This internal structure contains the analysis context for one PID.
  //!
  class PIDContext {
   public:
    bool receivingData = {};  //!< Incoming data should be processed or ignored.
    TransMode transMode = TRANSMODE_SERIAL;  //!< Teletext transmission mode.
    uint16_t currentPage = {};               //!< Current Teletext page number.
    TeletextPageMap
        pages;  //!< Working Teletext page buffers, indexed by page number.
    //!
    //! Default constructor.
    //!
    PIDContext() = default;
  };

 public:
  DvbTeletextParser() = default;
  ~DvbTeletextParser() = default;

  DvbTeletextParser(const DvbTeletextParser&) = delete;
  DvbTeletextParser& operator=(const DvbTeletextParser&) = delete;

  //!
  //! Process one Teletext packet.
  //! @param [in] pid PID number.
  //! @param [in,out] pc PID context.
  //! @param [in] dataUnitId Teletext packet data unit id.
  //! @param [in] pkt Address of Teletext packet (44 bytes,
  //! TELETEXT_PACKET_SIZE).
  //! @param [out] samples Resulting text samples.
  //!
  void ProcessTeletextPacket(uint32_t pid,
                             PIDContext& pc,
                             TeletextDataUnitId dataUnitId,
                             const uint8_t* pkt,
                             int64_t pts,
                             std::vector<std::shared_ptr<TextSample>>* samples);

  void Flush(uint32_t pid,
             PIDContext& pc,
             std::vector<std::shared_ptr<TextSample>>* samples);

 protected:
  //!
  //! Process one Teletext page.
  //! @param [in] pid PID number.
  //! @param [in,out] page Teletext page.
  //! @param [in] pageNumber Page number.
  //! @param [out] samples Resulting text samples.
  //! @param [in] add_colour Add color flag.
  //!
  static void ProcessTeletextPage(
      uint32_t pid,
      TeletextPage& page,
      uint16_t pageNumber,
      std::vector<std::shared_ptr<TextSample>>* samples,
      bool add_color = false);

  //!
  //! Remove 8/4 Hamming code.
  //! @param [in] a Hamming-encoded byte.
  //! @return Decoded byte.
  //! @see ETSI 300 706, section 8.2.
  //!
  static uint8_t unham_8_4(uint8_t a) noexcept;

  //!
  //! Remove 24/18 Hamming code.
  //! @param [in] a Hamming-encoded word.
  //! @return Decoded word.
  //! @see ETSI 300 706, section 8.3.
  //!
  static uint32_t unham_24_18(uint32_t a) noexcept;

  //!
  //! Extract Teletext magazine number from Teletext page.
  //! @param [in] page Teletext page.
  //! @return The Teletext magazine number.
  //!
  static int magazineOf(int page) noexcept { return (page >> 8) & 0x0F; }

  //!
  //! Extract Teletext page number from Teletext page.
  //! @param [in] page Teletext page.
  //! @return The Teletext page number.
  //!
  static int pageOf(int page) noexcept { return page & 0xFF; }

  //!
  //! Converts a page number from BCD to binary.
  //! Teletext page numbers are stored in Binary-Coded Decimal.
  //! @param [in] bcd BCD page number.
  //! @return Binary page number.
  //!
  static int pageBcdToBinary(int bcd) noexcept;

  //!
  //! Converts a page number from binary to BCD.
  //! Teletext page numbers are stored in Binary-Coded Decimal.
  //! @param [in] bin Binary page number.
  //! @return BCD page number.
  //!
  static int pageBinaryToBcd(int bin) noexcept;

 private:
  // friend class DvbTeletextParserTest;

  int64_t last_pts_ = {};
  bool addColors_ = {};  //!< Add font color tags.
};

}  // namespace media
}  // namespace shaka

#endif  // PACKAGER_MEDIA_DVB_DVB_TELETEXT_PARSER_H_
