//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2021, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//
// Parts of this module are inspired from Telxcc, a free open-source Teletext
// extractor from Petr Kutalek (https://github.com/petrkutalek/telxcc).
// Copyright: (c) 2011-2014 Forers, s. r. o.: telxcc
//
//----------------------------------------------------------------------------
//
// Relevant standards:
//
// - ETSI EN 300 472 V1.3.1 (2003-05)
//   Digital Video Broadcasting (DVB);
//   Specification for conveying ITU-R System B Teletext in DVB bitstreams
// - ETSI EN 300 706 V1.2.1 (2003-04)
//   Enhanced Teletext specification
// - ETSI EN 300 708 V1.2.1 (2003-04)
//   Television systems; Data transmission within Teletext
//
//----------------------------------------------------------------------------

#include "packager/media/formats/dvb/dvb_teletext_parser.h"

namespace shaka {
namespace media {

namespace {

//----------------------------------------------------------------------------
// General routine to convert from UTF-16 to UTF-8.
//----------------------------------------------------------------------------

void ConvertUTF16ToUTF8(const char16_t*& inStart,
                        const char16_t* inEnd,
                        char*& outStart,
                        char* outEnd) {
  uint32_t code;
  uint32_t high6;

  while (inStart < inEnd && outStart < outEnd) {
    // Get current code point as 16-bit value.
    code = *inStart++;

    // Get the higher 6 bits of the 16-bit value.
    high6 = code & 0xFC00;

    // The possible ranges are:
    // - 0x0000-0x0xD7FF : direct 16-bit code point.
    // - 0xD800-0x0xDBFF : leading surrogate, first part of a surrogate pair.
    // - 0xDC00-0x0xDFFF : trailing surrogate, second part of a surrogate pair,
    //                     invalid and ignored if encountered as first value.
    // - 0xE000-0x0xFFFF : direct 16-bit code point.

    if (high6 == 0xD800) {
      // This is a "leading surrogate", must be followed by a "trailing
      // surrogate".
      if (inStart >= inEnd) {
        // Invalid truncated input string, stop here.
        break;
      }
      // A surrogate pair always gives a code point value over 0x10000.
      // This will be encoded in UTF-8 using 4 bytes, check that we have room
      // for it.
      if (outStart + 4 > outEnd) {
        inStart--;  // Push back the leading surrogate into the input buffer.
        break;
      }
      // Get the "trailing surrogate".
      const uint32_t surr = *inStart++;
      // Ignore the code point if the leading surrogate is not in the valid
      // range.
      if ((surr & 0xFC00) == 0xDC00) {
        // Rebuild the 32-bit value of the code point.
        code = 0x010000 + (((code - 0xD800) << 10) | (surr - 0xDC00));
        // Encode it as 4 bytes in UTF-8.
        outStart[3] = char(0x80 | (code & 0x3F));
        code >>= 6;
        outStart[2] = char(0x80 | (code & 0x3F));
        code >>= 6;
        outStart[1] = char(0x80 | (code & 0x3F));
        code >>= 6;
        outStart[0] = char(0xF0 | (code & 0x07));
        outStart += 4;
      }
    }

    else if (high6 != 0xDC00) {
      // The 16-bit value is the code point.
      if (code < 0x0080) {
        // ASCII compatible value, one byte encoding.
        *outStart++ = char(code);
      } else if (code < 0x800 && outStart + 1 < outEnd) {
        // 2 bytes encoding.
        outStart[1] = char(0x80 | (code & 0x3F));
        code >>= 6;
        outStart[0] = char(0xC0 | (code & 0x1F));
        outStart += 2;
      } else if (code >= 0x800 && outStart + 2 < outEnd) {
        // 3 bytes encoding.
        outStart[2] = char(0x80 | (code & 0x3F));
        code >>= 6;
        outStart[1] = char(0x80 | (code & 0x3F));
        code >>= 6;
        outStart[0] = char(0xE0 | (code & 0x0F));
        outStart += 3;
      } else {
        // There not enough space in the output buffer.
        inStart--;  // Push back the leading surrogate into the input buffer.
        break;
      }
    }
  }
}

void toUTF8(const std::basic_string<char16_t>& in, std::string& utf8) {
  // The maximum number of UTF-8 bytes is 3 times the number of UTF-16 codes.
  utf8.resize(3 * in.size());

  const char16_t* inStart = in.data();
  char* outStart = const_cast<char*>(utf8.data());
  ConvertUTF16ToUTF8(inStart, inStart + in.size(), outStart,
                     outStart + utf8.size());

  utf8.resize(outStart - utf8.data());
}
}

//-----------------------------------------------------------------------------
// From various original sources.
//-----------------------------------------------------------------------------

namespace {

// Static table to remove 8/4 Hamming code.
const uint8_t UNHAM_8_4[256] = {
    0x01, 0xff, 0x01, 0x01, 0xff, 0x00, 0x01, 0xff, 0xff, 0x02, 0x01, 0xff,
    0x0a, 0xff, 0xff, 0x07, 0xff, 0x00, 0x01, 0xff, 0x00, 0x00, 0xff, 0x00,
    0x06, 0xff, 0xff, 0x0b, 0xff, 0x00, 0x03, 0xff, 0xff, 0x0c, 0x01, 0xff,
    0x04, 0xff, 0xff, 0x07, 0x06, 0xff, 0xff, 0x07, 0xff, 0x07, 0x07, 0x07,
    0x06, 0xff, 0xff, 0x05, 0xff, 0x00, 0x0d, 0xff, 0x06, 0x06, 0x06, 0xff,
    0x06, 0xff, 0xff, 0x07, 0xff, 0x02, 0x01, 0xff, 0x04, 0xff, 0xff, 0x09,
    0x02, 0x02, 0xff, 0x02, 0xff, 0x02, 0x03, 0xff, 0x08, 0xff, 0xff, 0x05,
    0xff, 0x00, 0x03, 0xff, 0xff, 0x02, 0x03, 0xff, 0x03, 0xff, 0x03, 0x03,
    0x04, 0xff, 0xff, 0x05, 0x04, 0x04, 0x04, 0xff, 0xff, 0x02, 0x0f, 0xff,
    0x04, 0xff, 0xff, 0x07, 0xff, 0x05, 0x05, 0x05, 0x04, 0xff, 0xff, 0x05,
    0x06, 0xff, 0xff, 0x05, 0xff, 0x0e, 0x03, 0xff, 0xff, 0x0c, 0x01, 0xff,
    0x0a, 0xff, 0xff, 0x09, 0x0a, 0xff, 0xff, 0x0b, 0x0a, 0x0a, 0x0a, 0xff,
    0x08, 0xff, 0xff, 0x0b, 0xff, 0x00, 0x0d, 0xff, 0xff, 0x0b, 0x0b, 0x0b,
    0x0a, 0xff, 0xff, 0x0b, 0x0c, 0x0c, 0xff, 0x0c, 0xff, 0x0c, 0x0d, 0xff,
    0xff, 0x0c, 0x0f, 0xff, 0x0a, 0xff, 0xff, 0x07, 0xff, 0x0c, 0x0d, 0xff,
    0x0d, 0xff, 0x0d, 0x0d, 0x06, 0xff, 0xff, 0x0b, 0xff, 0x0e, 0x0d, 0xff,
    0x08, 0xff, 0xff, 0x09, 0xff, 0x09, 0x09, 0x09, 0xff, 0x02, 0x0f, 0xff,
    0x0a, 0xff, 0xff, 0x09, 0x08, 0x08, 0x08, 0xff, 0x08, 0xff, 0xff, 0x09,
    0x08, 0xff, 0xff, 0x0b, 0xff, 0x0e, 0x03, 0xff, 0xff, 0x0c, 0x0f, 0xff,
    0x04, 0xff, 0xff, 0x09, 0x0f, 0xff, 0x0f, 0x0f, 0xff, 0x0e, 0x0f, 0xff,
    0x08, 0xff, 0xff, 0x05, 0xff, 0x0e, 0x0d, 0xff, 0xff, 0x0e, 0x0f, 0xff,
    0x0e, 0x0e, 0xff, 0x0e};

// Text foreground color codes.
const char16_t* const TELETEXT_COLORS[8] = {
    // 0=black, 1=red,      2=green,    3=yellow,   4=blue,     5=magenta,
    // 6=cyan,     7=white
    u"#000000", u"#ff0000", u"#00ff00", u"#ffff00",
    u"#0000ff", u"#ff00ff", u"#00ffff", u"#ffffff"};
}

void DvbTeletextParser::TeletextPage::reset(int64_t timestamp) noexcept {
  showTimestamp = timestamp;
  hideTimestamp = 0;
  tainted = false;
  std::memset(text, 0, sizeof(text));
}

bool DvbTeletextParser::TeletextPage::is_empty() noexcept {
  // Optimization: slicing column by column -- higher probability we could find
  // boxed area start mark sooner
  for (std::size_t col = 0; col < kColumnsCount; col++) {
    for (std::size_t row = 1; row < kRowsCount; row++) {
      if (text[row][col] == 0x0B) {
        return false;
      }
    }
  }

  return true;
}

uint8_t DvbTeletextParser::unham_8_4(uint8_t a) noexcept {
  const uint8_t r = UNHAM_8_4[a];
  return r == 0xFF ? 0x00 : r & 0x0F;
}

uint32_t DvbTeletextParser::unham_24_18(uint32_t a) noexcept {
  uint8_t test = 0;

  // Tests A-F correspond to bits 0-6 respectively in 'test'.
  for (uint8_t i = 0; i < 23; i++) {
    test ^= ((a >> i) & 0x01) * (i + 33);
  }

  // Only parity bit is tested for bit 24
  test ^= ((a >> 23) & 0x01) * 32;

  if ((test & 0x1F) != 0x1F) {
    // Not all tests A-E correct
    if ((test & 0x20) == 0x20) {
      // F correct: Double error
      return 0xFFFFFFFF;
    }
    // Test F incorrect: Single error
    a ^= 1 << (30 - test);
  }

  return (a & 0x000004) >> 2 | (a & 0x000070) >> 3 | (a & 0x007F00) >> 4 |
         (a & 0x7F0000) >> 5;
}

int DvbTeletextParser::pageBcdToBinary(int bcd) noexcept {
  return 100 * ((bcd >> 8) & 0x0F) + 10 * ((bcd >> 4) & 0x0F) + (bcd & 0x0F);
}

int DvbTeletextParser::pageBinaryToBcd(int bin) noexcept {
  return (((bin / 100) % 10) << 8) | (((bin / 10) % 10) << 4) | (bin % 10);
}

void DvbTeletextParser::ProcessTeletextPacket(
    uint32_t pid,
    PIDContext& pc,
    TeletextDataUnitId dataUnitId,
    const uint8_t* pkt,
    int64_t pts,
    std::vector<std::shared_ptr<TextSample>>* samples) {
  last_pts_ = pts;

  // Structure of a Teletext packet. See ETSI 300 706, section 7.1.
  // - Clock run-in: 1 byte
  // - Framing code: 1 byte
  // - Address: 2 bytes
  // - Data: 40 bytes

  // Variable names conform to ETS 300 706, chapter 7.1.2.
  uint8_t address = uint8_t(unham_8_4(pkt[3]) << 4) | unham_8_4(pkt[2]);
  uint8_t m = address & 0x07;
  if (m == 0) {
    m = 8;
  }
  uint8_t y = (address >> 3) & 0x1F;
  const uint8_t* data = pkt + 4;
  uint8_t designationCode = (y > 25) ? unham_8_4(data[0]) : 0x00;

  if (y == 0) {
    // Page number and control bits
    uint16_t pageNumber = uint16_t(uint16_t(m) << 8) |
                          uint16_t(uint16_t(unham_8_4(data[1])) << 4) |
                          unham_8_4(data[0]);
    uint8_t charset =
        ((unham_8_4(data[7]) & 0x08) | (unham_8_4(data[7]) & 0x04) |
         (unham_8_4(data[7]) & 0x02)) >>
        1;

    // ETS 300 706, chapter 9.3.1.3:
    //
    // When set to '1' the service is designated to be in Serial mode and the
    // transmission
    // of a page is terminated by the next page header with a different page
    // number.
    // When set to '0' the service is designated to be in Parallel mode and the
    // transmission
    // of a page is terminated by the next page header with a different page
    // number but the
    // same magazine number.
    //
    // The same setting shall be used for all page headers in the service.
    //
    // ETS 300 706, chapter 7.2.1: Page is terminated by and excludes the next
    // page header packet
    // having the same magazine address in parallel transmission mode, or any
    // magazine address in
    // serial transmission mode.
    //
    pc.transMode = TransMode(unham_8_4(data[7]) & 0x01);

    // FIXME: Well, this is not ETS 300 706 kosher, however we are interested in
    // TELETEXT_SUBTITLE only
    if (pc.transMode == TRANSMODE_PARALLEL &&
        dataUnitId != TeletextDataUnitId::SUBTITLE) {
      return;
    }

    if (pc.receivingData && ((pc.transMode == TRANSMODE_SERIAL &&
                              pageOf(pageNumber) != pageOf(pc.currentPage)) ||
                             (pc.transMode == TRANSMODE_PARALLEL &&
                              pageOf(pageNumber) != pageOf(pc.currentPage) &&
                              m == magazineOf(pc.currentPage)))) {
      pc.receivingData = false;
    }

    // A new frame starts on a page. If this page had a non-empty frame in
    // progress, flush it now.
    TeletextPage& page(pc.pages[pageNumber]);
    if (page.tainted) {
      // It would not be nice if subtitle hides previous video frame, so we
      // contract 40 ms (1 frame @25 fps)
      page.hideTimestamp = pts - 40;
      ProcessTeletextPage(pid, page, pageNumber, samples, addColors_);
    }

    // Start new page.
    pc.currentPage = pageNumber;
    page.reset(pts);
    page.charset.resetX28(charset);
    pc.receivingData = true;
  } else if (m == magazineOf(pc.currentPage) && y >= 1 && y <= 23 &&
             pc.receivingData) {
    // ETS 300 706, chapter 9.4.1: Packets X/26 at presentation Levels 1.5, 2.5,
    // 3.5 are used for addressing
    // a character location and overwriting the existing character defined on
    // the Level 1 page
    // ETS 300 706, annex B.2.2: Packets with Y = 26 shall be transmitted before
    // any packets with Y = 1 to Y = 25;
    // so page_buffer.text[y][i] may already contain any character received
    // in frame number 26, skip original G0 character
    TeletextPage& page(pc.pages[pc.currentPage]);
    for (uint8_t i = 0; i < 40; i++) {
      if (page.text[y][i] == 0x00) {
        page.text[y][i] = page.charset.teletextToUcs2(data[i]);
      }
    }
    page.tainted = true;
  } else if (m == magazineOf(pc.currentPage) && y == 26 && pc.receivingData) {
    // ETS 300 706, chapter 12.3.2: X/26 definition
    uint32_t x26Row = 0;
    uint32_t x26Col = 0;

    uint32_t triplets[13] = {0};
    for (uint8_t i = 1, j = 0; i < 40; i += 3, j++) {
      triplets[j] =
          unham_24_18((data[i + 2] << 16) | (data[i + 1] << 8) | data[i]);
    }

    for (uint8_t j = 0; j < 13; j++) {
      if (triplets[j] == 0xffffffff) {
        // invalid data (HAM24/18 uncorrectable error detected), skip group
        continue;
      }

      const uint8_t tdata = uint8_t((triplets[j] & 0x3f800) >> 11);
      const uint8_t tmode = uint8_t((triplets[j] & 0x7c0) >> 6);
      const uint8_t taddr = uint8_t(triplets[j] & 0x3f);
      const bool rowAddressGroup = (taddr >= 40) && (taddr <= 63);

      TeletextPage& page(pc.pages[pc.currentPage]);

      // ETS 300 706, chapter 12.3.1, table 27: set active position
      if (tmode == 0x04 && rowAddressGroup) {
        x26Row = taddr - 40;
        if (x26Row == 0) {
          x26Row = 24;
        }
        x26Col = 0;
      }

      // ETS 300 706, chapter 12.3.1, table 27: termination marker
      if (tmode >= 0x11 && tmode <= 0x1f && rowAddressGroup) {
        break;
      }

      // ETS 300 706, chapter 12.3.1, table 27: character from G2 set
      if (tmode == 0x0f && !rowAddressGroup) {
        x26Col = taddr;
        if (tdata > 31) {
          page.text[x26Row][x26Col] = page.charset.g2ToUcs2(tdata);
        }
      }

      // ETS 300 706, chapter 12.3.1, table 27: G0 character with diacritical
      // mark
      if (tmode >= 0x11 && tmode <= 0x1f && !rowAddressGroup) {
        x26Col = taddr;
        page.text[x26Row][x26Col] =
            page.charset.g2AccentToUcs2(tdata, tmode - 0x11);
      }
    }
  } else if (m == magazineOf(pc.currentPage) && y == 28 && pc.receivingData) {
    // TODO:
    //   ETS 300 706, chapter 9.4.7: Packet X/28/4
    //   Where packets 28/0 and 28/4 are both transmitted as part of a page,
    //   packet 28/0 takes precedence over 28/4 for all but the colour map entry
    //   coding.
    if (designationCode == 0 || designationCode == 4) {
      // ETS 300 706, chapter 9.4.2: Packet X/28/0 Format 1
      // ETS 300 706, chapter 9.4.7: Packet X/28/4
      const uint32_t triplet0 =
          unham_24_18((data[3] << 16) | (data[2] << 8) | data[1]);
      // ETS 300 706, chapter 9.4.2: Packet X/28/0 Format 1 only
      if ((triplet0 & 0x0f) == 0x00) {
        pc.pages[pc.currentPage].charset.setG0Charset(triplet0);
        pc.pages[pc.currentPage].charset.setX28(
            uint8_t((triplet0 & 0x3f80) >> 7));
      }
    }
  } else if (m == magazineOf(pc.currentPage) && y == 29) {
    // TODO:
    //   ETS 300 706, chapter 9.5.1 Packet M/29/0
    //   Where M/29/0 and M/29/4 are transmitted for the same magazine, M/29/0
    //   takes precedence over M/29/4.
    if (designationCode == 0 || designationCode == 4) {
      // ETS 300 706, chapter 9.5.1: Packet M/29/0
      // ETS 300 706, chapter 9.5.3: Packet M/29/4
      const uint32_t triplet0 =
          unham_24_18((data[3] << 16) | (data[2] << 8) | data[1]);
      // ETS 300 706, table 11: Coding of Packet M/29/0
      // ETS 300 706, table 13: Coding of Packet M/29/4
      if ((triplet0 & 0xff) == 0x00) {
        pc.pages[pc.currentPage].charset.setG0Charset(triplet0);
        pc.pages[pc.currentPage].charset.setM29(
            uint8_t((triplet0 & 0x3f80) >> 7));
      }
    }
  } else if ((m == 8) && (y == 30)) {
    // ETS 300 706, chapter 9.8: Broadcast Service Data Packets.
    // We can find here "Programme Identification Data" and absolute data / time
    // stamps.
    // It is not interesting for us.
  }
}

void DvbTeletextParser::Flush(
    uint32_t pid,
    PIDContext& pc,
    std::vector<std::shared_ptr<TextSample>>* samples) {
  for (auto& page : pc.pages) {
    if (page.second.tainted) {
      // TODO: calc last pts
      // Use the last timestamp (ms) for end of message.
      // const MilliSecond ms = pidDuration(itPid->first);
      // This time, we do not subtract any frames, there will be no more frames.
      page.second.hideTimestamp = last_pts_;

      ProcessTeletextPage(pid, page.second, page.first, samples, addColors_);

      page.second.reset(last_pts_);
    }
  }
}

void DvbTeletextParser::ProcessTeletextPage(
    uint32_t pid,
    TeletextPage& page,
    uint16_t pageNumber,
    std::vector<std::shared_ptr<TextSample>>* samples,
    bool add_colour) {
  if (page.is_empty()) {
    return;
  }

  // Adjust frame count and timestamps.
  page.frameCount++;
  if (page.showTimestamp > page.hideTimestamp) {
    page.hideTimestamp = page.showTimestamp;
  }

  // Prepare the Teletext frame.
  // DvbTeletextFrame frame(pid, pageBcdToBinary(pageNumber), page.frameCount,
  // page.showTimestamp, page.hideTimestamp);

  // Prepare the teletext frame text.
  TextFragment fragment;
  std::basic_string<char16_t> line;
  std::string line_utf8;

  // Process page data.
  for (std::size_t row = 1; row < TeletextPage::kRowsCount; row++) {
    line.clear();
    line_utf8.clear();

    // Anchors for string trimming purpose
    std::size_t colStart = TeletextPage::kColumnsCount;
    std::size_t colStop = TeletextPage::kColumnsCount;

    std::size_t col = TeletextPage::kColumnsCount;
    do {
      col--;

      if (page.text[row][col] == 0x0B) {
        colStart = col;
        break;
      }
    } while (col != 0);

    if (colStart == TeletextPage::kColumnsCount) {
      // Line is empty
      continue;
    }

    for (std::size_t col = colStart + 1; col < TeletextPage::kColumnsCount;
         col++) {
      if (page.text[row][col] > 0x20) {
        if (colStop == TeletextPage::kColumnsCount) {
          colStart = col;
        }
        colStop = col;
      }
      if (page.text[row][col] == 0x0A) {
        break;
      }
    }

    if (colStop == TeletextPage::kColumnsCount) {
      // Line is empty
      continue;
    }

    // ETS 300 706, chapter 12.2: Alpha White ("Set-After") - Start-of-row
    // default condition.
    // used for colour changes _before_ start box mark
    // white is default as stated in ETS 300 706, chapter 12.2
    // black(0), red(1), green(2), yellow(3), blue(4), magenta(5), cyan(6),
    // white(7)
    uint16_t foregroundColor = 0x07;
    bool fontTagOpened = false;

    for (std::size_t col = 0; col <= colStop; col++) {
      // v is just a shortcut
      char16_t v = page.text[row][col];

      if (col < colStart) {
        if (v <= 0x7) {
          foregroundColor = v;
        }
      }

      if (col == colStart) {
        if (foregroundColor != 0x7 && add_colour) {
          line.append(u"<font color=\"");
          line.append(TELETEXT_COLORS[foregroundColor]);
          line.append(u"\">");
          fontTagOpened = true;
        }
      }

      if (col >= colStart) {
        if (v <= 0x7) {
          // ETS 300 706, chapter 12.2: Unless operating in "Hold Mosaics" mode,
          // each character space occupied by a spacing attribute is displayed
          // as a SPACE.
          if (add_colour) {
            if (fontTagOpened) {
              line.append(u"</font> ");
              fontTagOpened = false;
            }

            // <font/> tags only when needed
            if (v > 0x00 && v < 0x07) {
              line.append(u"<font color=\"");
              line.append(TELETEXT_COLORS[v]);
              line.append(u"\">");
              fontTagOpened = true;
            }
          } else {
            v = 0x20;
          }
        }

        // Translate some chars into entities, if in colour mode, to replace
        // unsafe HTML tag chars
        if (v >= 0x20 && add_colour) {
          struct HtmlEntity {
            char16_t character;
            const char16_t* entity;
          };
          static const HtmlEntity entities[] = {
              {u'<', u"&lt;"}, {u'>', u"&gt;"}, {u'&', u"&amp;"}, {0, nullptr}};
          for (const HtmlEntity* p = entities; p->entity != nullptr; ++p) {
            if (v == p->character) {
              line.append(p->entity);
              v = 0;  // v < 0x20 won't be printed in next block
              break;
            }
          }
        }

        if (v >= 0x20) {
          line += v;
        }
      }
    }

    // No tag will be left opened!
    if (add_colour && fontTagOpened) {
      line.append(u"</font>");
      fontTagOpened = false;
    }

    // Line is now complete.
    toUTF8(line, line_utf8);

    if (!line_utf8.empty()) {
      if (!fragment.body.empty()) {
        fragment.body.append(" ");
      }

      fragment.body.append(line_utf8);
    }
  }

  samples->emplace_back(std::make_shared<TextSample>(
      std::to_string(page.frameCount), page.showTimestamp, page.hideTimestamp,
      TextSettings{}, fragment));
}
}
}