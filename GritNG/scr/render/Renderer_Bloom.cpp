#include "Renderer.h"
#include "Renderer_Internal.h"
#include <bgfx/bgfx.h>
#include <bx/math.h>

using grit::render::detail::submitFullscreenTriangle;

namespace grit::render {

    void Renderer::renderBloom(bool& outBloomReady)
    {
        outBloomReady = m_bloomEnabled
            && m_bloomBrightEntry && bgfx::isValid(m_bloomBrightEntry->program)
            && m_bloomDownsampleEntry && bgfx::isValid(m_bloomDownsampleEntry->program)
            && m_bloomUpsampleEntry && bgfx::isValid(m_bloomUpsampleEntry->program)
            && bgfx::isValid(m_bloomFB[0]);

        if (!outBloomReady) return;

        float identity[16];
        bx::mtxIdentity(identity);

        // Bright pass: HDR -> bloom[0]
        {
            bgfx::ViewId vid = kViewBloomBright;
            bgfx::setViewName(vid, "BloomBright");
            bgfx::setViewFrameBuffer(vid, m_bloomFB[0]);
            bgfx::setViewRect(vid, 0, 0, m_bloomWidth[0], m_bloomHeight[0]);
            bgfx::setViewClear(vid, BGFX_CLEAR_COLOR, 0x00000000, 1.0f, 0);
            bgfx::setViewTransform(vid, identity, identity);

            float params[4] = { m_bloomThreshold, m_bloomKnee, m_bloomIntensity, 0.f };
            bgfx::setUniform(m_uBloomParams, params);
            bgfx::setTexture(0, m_uBloomSource, m_vpColor);
            submitFullscreenTriangle(vid, m_bloomBrightEntry->program,
                BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
        }

        // Downsample
        for (int i = 0; i + 1 < kBloomMipCount; ++i) {
            bgfx::ViewId vid = (bgfx::ViewId)(kViewBloomDownsample0 + i);
            bgfx::setViewName(vid, "BloomDown");
            bgfx::setViewFrameBuffer(vid, m_bloomFB[i + 1]);
            bgfx::setViewRect(vid, 0, 0, m_bloomWidth[i + 1], m_bloomHeight[i + 1]);
            bgfx::setViewClear(vid, BGFX_CLEAR_COLOR, 0x00000000, 1.0f, 0);
            bgfx::setViewTransform(vid, identity, identity);

            float texel[4] = { 1.f / (float)m_bloomWidth[i], 1.f / (float)m_bloomHeight[i], 0.f, 0.f };
            bgfx::setUniform(m_uBloomTexel, texel);
            bgfx::setTexture(0, m_uBloomSource, m_bloomTex[i]);
            submitFullscreenTriangle(vid, m_bloomDownsampleEntry->program,
                BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
        }

        // Upsample (additive)
        for (int i = kBloomMipCount - 2; i >= 0; --i) {
            bgfx::ViewId vid = (bgfx::ViewId)(kViewBloomUpsample0 + (kBloomMipCount - 2 - i));
            bgfx::setViewName(vid, "BloomUp");
            bgfx::setViewFrameBuffer(vid, m_bloomFB[i]);
            bgfx::setViewRect(vid, 0, 0, m_bloomWidth[i], m_bloomHeight[i]);
            bgfx::setViewTransform(vid, identity, identity);

            float texel[4] = {
                1.f / (float)m_bloomWidth[i + 1], 1.f / (float)m_bloomHeight[i + 1],
                m_bloomFilterRadius, 0.f };
            bgfx::setUniform(m_uBloomTexel, texel);
            bgfx::setTexture(0, m_uBloomSource, m_bloomTex[i + 1]);

            uint64_t state = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A
                | BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_ONE);
            submitFullscreenTriangle(vid, m_bloomUpsampleEntry->program, state);
        }
    }

    void Renderer::renderTonemap(bool bloomReady)
    {
        if (!(m_tonemapEntry && bgfx::isValid(m_tonemapEntry->program)
            && bgfx::isValid(m_vpFBLDR) && bgfx::isValid(m_vpColor)))
            return;

        bgfx::setViewName(kViewTonemap, "Tonemap");
        bgfx::setViewFrameBuffer(kViewTonemap, m_vpFBLDR);
        bgfx::setViewRect(kViewTonemap, 0, 0, (uint16_t)m_vpWidth, (uint16_t)m_vpHeight);
        bgfx::setViewClear(kViewTonemap, BGFX_CLEAR_COLOR, 0x000000ff, 1.0f, 0);

        float identity[16];
        bx::mtxIdentity(identity);
        bgfx::setViewTransform(kViewTonemap, identity, identity);

        float params[4] = {
            m_exposure, (float)m_tonemapOperator, m_gamma,
            bloomReady ? m_bloomIntensity : 0.f };
        bgfx::setUniform(m_uTonemapParams, params);
        bgfx::setTexture(0, m_uHdrTexSampler, m_vpColor);
        bgfx::setTexture(1, m_uBloomSampler, bloomReady ? m_bloomTex[0] : m_blackPixel);

        submitFullscreenTriangle(kViewTonemap, m_tonemapEntry->program,
            BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
    }

} // namespace grit::render