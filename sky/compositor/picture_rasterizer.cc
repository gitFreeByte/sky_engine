// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sky/compositor/compositor_options.h"
#include "sky/compositor/checkerboard.h"
#include "sky/compositor/picture_rasterizer.h"
#include "sky/compositor/paint_context.h"
#include "base/logging.h"
#include "third_party/skia/include/core/SkPicture.h"
#include "third_party/skia/include/gpu/GrContext.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "third_party/skia/include/core/SkCanvas.h"

namespace sky {
namespace compositor {

PictureRasterzier::PictureRasterzier() {
}

PictureRasterzier::~PictureRasterzier() {
}

static void ImageReleaseProc(SkImage::ReleaseContext texture) {
  DCHECK(texture);
  reinterpret_cast<GrTexture*>(texture)->unref();
}

PictureRasterzier::Key::Key(uint32_t ident, SkISize sz)
    : pictureID(ident), size(sz){};

PictureRasterzier::Key::Key(const Key& key) = default;

PictureRasterzier::Value::Value()
    : access_count(kDeadAccessCount), image(nullptr) {
}

PictureRasterzier::Value::~Value() {
}

RefPtr<SkImage> PictureRasterzier::ImageFromPicture(
    PaintContext& context,
    GrContext* gr_context,
    SkPicture* picture,
    const SkISize& physical_size,
    const SkMatrix& incoming_ctm) {
  // Step 1: Create a texture from the context's texture provider

  GrSurfaceDesc surfaceDesc;
  surfaceDesc.fWidth = physical_size.width();
  surfaceDesc.fHeight = physical_size.height();
  surfaceDesc.fFlags = kRenderTarget_GrSurfaceFlag;
  surfaceDesc.fConfig = kRGBA_8888_GrPixelConfig;

  GrTexture* texture =
      gr_context->textureProvider()->createTexture(surfaceDesc, true);

  if (!texture) {
    // The texture provider could not allocate a texture backing. Render
    // directly to the surface from the picture till the memory pressure
    // subsides
    return nullptr;
  }

  // Step 2: Create a backend render target description for the created texture

  GrBackendTextureDesc textureDesc;
  textureDesc.fConfig = surfaceDesc.fConfig;
  textureDesc.fWidth = physical_size.width() / incoming_ctm.getScaleX();
  textureDesc.fHeight = physical_size.height() / incoming_ctm.getScaleY();
  textureDesc.fSampleCnt = surfaceDesc.fSampleCnt;
  textureDesc.fFlags = kRenderTarget_GrBackendTextureFlag;
  textureDesc.fConfig = surfaceDesc.fConfig;
  textureDesc.fTextureHandle = texture->getTextureHandle();

  // Step 3: Render the picture into the offscreen texture

  GrRenderTarget* renderTarget = texture->asRenderTarget();
  DCHECK(renderTarget);

  PassRefPtr<SkSurface> surface =
      adoptRef(SkSurface::NewRenderTargetDirect(renderTarget));
  DCHECK(surface);

  SkCanvas* canvas = surface->getCanvas();
  DCHECK(canvas);

  canvas->setMatrix(
      SkMatrix::MakeScale(incoming_ctm.getScaleX(), incoming_ctm.getScaleY()));
  canvas->drawPicture(picture);

  if (context.options().isEnabled(
          CompositorOptions::Option::HightlightRasterizedImages)) {
    DrawCheckerboard(canvas, textureDesc.fWidth, textureDesc.fHeight);
  }

  // Step 4: Create an image representation from the texture

  RefPtr<SkImage> image = adoptRef(
      SkImage::NewFromTexture(gr_context, textureDesc, kPremul_SkAlphaType,
                              &ImageReleaseProc, texture));

  if (image) {
    cache_fills_.increment();
  }

  return image;
}

RefPtr<SkImage> PictureRasterzier::GetCachedImageIfPresent(
    PaintContext& context,
    GrContext* gr_context,
    SkPicture* picture,
    const SkISize& physical_size,
    const SkMatrix& incoming_ctm) {
  if (physical_size.isEmpty() || picture == nullptr || gr_context == nullptr) {
    return nullptr;
  }

  const Key key(picture->uniqueID(), physical_size);

  Value& value = cache_[key];

  if (value.access_count == Value::kDeadAccessCount) {
    value.access_count = 1;
    return nullptr;
  }

  value.access_count++;
  DCHECK(value.access_count == 1)
      << "Did you forget to call PurgeCache between frames?";

  if (!value.image) {
    value.image = ImageFromPicture(context, gr_context, picture, physical_size,
                                   incoming_ctm);
  }

  if (value.image) {
    cache_hits_.increment();
  }

  return value.image;
}

void PictureRasterzier::PurgeCache() {
  std::unordered_set<Key, KeyHash, KeyEqual> keys_to_purge;

  for (auto& item : cache_) {
    const auto count = --item.second.access_count;
    if (count == Value::kDeadAccessCount) {
      keys_to_purge.insert(item.first);
    }
  }

  cache_evictions_.increment(keys_to_purge.size());

  for (const auto& key : keys_to_purge) {
    cache_.erase(key);
  }
}

}  // namespace compositor
}  // namespace sky
