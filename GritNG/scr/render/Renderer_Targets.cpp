#include "Renderer.h"
#include "../rhi/Device.h"
#include "../core/Log.h"
#include <bgfx/bgfx.h>

namespace grit::render {

    void Renderer::createViewportTargets(u32 w, u32 h)
    {
        if (w == 0 || h == 0) return;
        if (w == m_vpWidth && h == m_vpHeight && bgfx::isValid(m_vpFB)) return;

        destroyViewportTargets();

        const uint64_t colorFlags =
            BGFX_TEXTURE_RT
            | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP
            | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT;
        const uint64_t depthFlags = BGFX_TEXTURE_RT_WRITE_ONLY;

        m_vpColor = bgfx::createTexture2D(
            (uint16_t)w, (uint16_t)h, false, 1,
            bgfx::TextureFormat::RGBA16F, colorFlags);

        m_vpDepth = bgfx::createTexture2D(
            (uint16_t)w, (uint16_t)h, false, 1,
            bgfx::TextureFormat::D24S8, depthFlags);

        bgfx::TextureHandle atts[2] = { m_vpColor, m_vpDepth };
        m_vpFB = bgfx::createFrameBuffer(2, atts, false);

        m_vpColorLDR = bgfx::createTexture2D(
            (uint16_t)w, (uint16_t)h, false, 1,
            bgfx::TextureFormat::BGRA8, colorFlags);

        bgfx::TextureHandle ldrAtts[1] = { m_vpColorLDR };
        m_vpFBLDR = bgfx::createFrameBuffer(1, ldrAtts, false);

        m_vpWidth = w;
        m_vpHeight = h;

        createBloomTargets(w, h);
    }

    void Renderer::createBloomTargets(u32 baseW, u32 baseH)
    {
        destroyBloomTargets();

        const uint64_t flags =
            BGFX_TEXTURE_RT
            | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP
            | BGFX_SAMPLER_MIN_ANISOTROPIC | BGFX_SAMPLER_MAG_ANISOTROPIC;

        uint16_t w = (uint16_t)(baseW / 2);
        uint16_t h = (uint16_t)(baseH / 2);

        for (int i = 0; i < kBloomMipCount; ++i) {
            if (w < 2) w = 2;
            if (h < 2) h = 2;

            m_bloomWidth[i] = w;
            m_bloomHeight[i] = h;

            m_bloomTex[i] = bgfx::createTexture2D(
                w, h, false, 1, bgfx::TextureFormat::RGBA16F, flags);

            bgfx::TextureHandle atts[1] = { m_bloomTex[i] };
            m_bloomFB[i] = bgfx::createFrameBuffer(1, atts, false);

            w = (uint16_t)(w / 2);
            h = (uint16_t)(h / 2);
        }
        m_bloomTargetsCreated = true;
    }

    void Renderer::destroyBloomTargets()
    {
        for (int i = 0; i < kBloomMipCount; ++i) {
            if (bgfx::isValid(m_bloomFB[i])) { bgfx::destroy(m_bloomFB[i]);  m_bloomFB[i] = BGFX_INVALID_HANDLE; }
            if (bgfx::isValid(m_bloomTex[i])) { bgfx::destroy(m_bloomTex[i]); m_bloomTex[i] = BGFX_INVALID_HANDLE; }
            m_bloomWidth[i] = m_bloomHeight[i] = 0;
        }
        if (!m_bloomTargetsCreated) return;
    }

    void Renderer::destroyViewportTargets()
    {
        destroyBloomTargets();

        if (bgfx::isValid(m_vpFBLDR)) { bgfx::destroy(m_vpFBLDR);    m_vpFBLDR = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_vpColorLDR)) { bgfx::destroy(m_vpColorLDR); m_vpColorLDR = BGFX_INVALID_HANDLE; }

        if (bgfx::isValid(m_vpFB)) { bgfx::destroy(m_vpFB);    m_vpFB = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_vpColor)) { bgfx::destroy(m_vpColor); m_vpColor = BGFX_INVALID_HANDLE; }
        if (bgfx::isValid(m_vpDepth)) { bgfx::destroy(m_vpDepth); m_vpDepth = BGFX_INVALID_HANDLE; }

        m_vpWidth = m_vpHeight = 0;
    }

    void Renderer::resizeViewport(u32 w, u32 h)
    {
        createViewportTargets(w, h);
    }

} // namespace grit::render