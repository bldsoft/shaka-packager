# Copyright 2021 Google LLC. All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

{
  'variables': {
    'shaka_code': 1,
  },
  'targets': [
    {
      'target_name': 'ocr',
      'type': '<(component)',
      'sources': [
        'public/status.h',
        'public/text_extractor_builder.h',
        'public/text_extractor.h',
      ],
      'dependencies': [
        '../base/base.gyp:base',
        '../packager.gyp:export',
      ],
      'defines': [
        'OCR_ENABLED',
      ],
    },
  ],
}
