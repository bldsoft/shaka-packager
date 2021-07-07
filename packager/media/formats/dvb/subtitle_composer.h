// Copyright 2020 Google LLC. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef PACKAGER_MEDIA_DVB_SUBTITLE_COMPOSER_H_
#define PACKAGER_MEDIA_DVB_SUBTITLE_COMPOSER_H_

#include <memory>
#include <unordered_map>
#include <vector>

#include "packager/base/macros.h"
#include "packager/media/base/text_sample.h"
#include "packager/media/formats/dvb/dvb_image.h"
#include "packager/ocr/public/text_extractor.h"

namespace shaka {
namespace ocr {

std::ostream& operator<<(std::ostream& os, const ocr::Status& x);
}
namespace media {

/// Holds pixel/caption data for a single DVB-sub page.  This composes
/// multiple objects and creates TextSample objects from it.
class SubtitleComposer {
 private:
  SubtitleComposer(uint16_t display_width,
                   uint16_t display_height,
                   std::unique_ptr<ocr::TextExtractor> text_extracor);

 public:
  SubtitleComposer();
  SubtitleComposer(std::unique_ptr<ocr::TextExtractor> text_extractor);
  ~SubtitleComposer();

  DISALLOW_COPY_AND_ASSIGN(SubtitleComposer);

  void SetDisplaySize(uint16_t width, uint16_t height);
  bool SetRegionPosition(uint8_t region_id, uint16_t x, uint16_t y);
  bool SetRegionInfo(uint8_t region_id,
                     uint8_t color_space_id,
                     uint16_t width,
                     uint16_t height);
  bool SetObjectInfo(uint16_t object_id,
                     uint8_t region_id,
                     uint16_t x,
                     uint16_t y,
                     int default_color_code);

  DvbImageColorSpace* GetColorSpace(uint8_t color_space_id);
  DvbImageColorSpace* GetColorSpaceForObject(uint16_t object_id);
  DvbImageBuilder* GetObjectImage(uint16_t object_id);

  bool GetSamples(int64_t start,
                  int64_t end,
                  std::vector<std::shared_ptr<TextSample>>* samples) const;
  void ClearObjects();

 private:
  struct RegionInfo {
    DvbImageColorSpace* color_space = nullptr;
    uint16_t x = 0;
    uint16_t y = 0;
    uint16_t width = 0;
    uint16_t height = 0;
  };

  struct ObjectInfo {
    RegionInfo* region = nullptr;
    int default_color_code = -1;
    uint16_t x = 0;
    uint16_t y = 0;
  };

  // Maps of IDs to their respective object.
  std::unordered_map<uint8_t, RegionInfo> regions_;
  std::unordered_map<uint8_t, DvbImageColorSpace> color_spaces_;
  std::unordered_map<uint16_t, ObjectInfo> objects_;
  std::unordered_map<uint16_t, DvbImageBuilder> images_;  // Uses object_id.
  uint16_t display_width_;
  uint16_t display_height_;

  std::unique_ptr<ocr::TextExtractor> text_extracor_;
};

}  // namespace media
}  // namespace shaka

#endif  // PACKAGER_MEDIA_DVB_SUBTITLE_COMPOSER_H_
