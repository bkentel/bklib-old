#include "pch.hpp"

#include "platform/win/com/com.hpp"
#include "gfx/renderer/renderer2d/renderer2d.hpp"

namespace g2d = ::bklib::gfx2d;
namespace win = ::bklib::platform::win;

namespace {
    D2D1_RECT_F make_rect(g2d::rect r) {
        return D2D1::RectF(r.left, r.top, r.right, r.bottom);
    }
}

struct brush_impl : public g2d::brush {
    win::com_ptr<ID2D1Brush> brush;
};

struct solid_color_brush_impl : public g2d::solid_color_brush {
    solid_color_brush_impl() {}

    explicit solid_color_brush_impl(win::com_ptr<ID2D1SolidColorBrush> brush)
        : brush(std::move(brush))
    {
    }

    win::com_ptr<ID2D1SolidColorBrush> brush;

    g2d::color get_color() const {
        return g2d::color(0.0, 0.0, 0.0);
    }

    void set_color(g2d::color const& color) {
        brush->SetColor(D2D1::ColorF(color.r, color.g, color.b, color.a));
    }
};

struct g2d::renderer::impl_t {
    impl_t(window& win) {
        HWND hwnd = win.handle();

        factory_ = win::make_com_ptr([](ID2D1Factory** out) {
            return D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, out);
        });

        RECT r;
        ::GetClientRect(hwnd, &r);

        D2D1_SIZE_U const size = D2D1::SizeU(
            r.right  - r.left,
            r.bottom - r.top
        );

        target_ = win::make_com_ptr([&](ID2D1HwndRenderTarget** out) {
            return factory_->CreateHwndRenderTarget(
                D2D1::RenderTargetProperties(),
                D2D1::HwndRenderTargetProperties(hwnd, size, ::D2D1_PRESENT_OPTIONS_IMMEDIATELY),
                out
            );
        });

        solid_brush_.brush = win::make_com_ptr([&](ID2D1SolidColorBrush** out) {
            return target_->CreateSolidColorBrush(
                D2D1::ColorF(D2D1::ColorF::Silver), out
            );
        });

        write_factory_ = win::make_com_ptr([&](IDWriteFactory** out) {
            return DWriteCreateFactory(
                DWRITE_FACTORY_TYPE_SHARED,
                __uuidof(IDWriteFactory),
                reinterpret_cast<IUnknown**>(out)
            );
        });

        text_format_ = win::make_com_ptr([&](IDWriteTextFormat** out) {
            return write_factory_->CreateTextFormat(
                L"Meiryo",
                nullptr,
                DWRITE_FONT_WEIGHT_NORMAL,
                DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL,
                16,
                L"",
                out
            );
        });

        //top left
        text_format_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        text_format_->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

        DWRITE_TRIMMING const trim = {
            DWRITE_TRIMMING_GRANULARITY_CHARACTER,
            0, 0
        };
        text_format_->SetTrimming(&trim, nullptr);

        target_->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
    }

    void
    update()
    {
        ::InvalidateRect(target_->GetHwnd(), nullptr, FALSE);
    }

    void
    resize(unsigned w, unsigned h)
    {
        target_->Resize(D2D1::SizeU(w, h));
    }

    g2d::solid_color_brush&
    get_solid_brush()
    {
        return solid_brush_;
    }

    void begin() {
        target_->BeginDraw();
    }

    void end() {
        target_->EndDraw();
    }

    void clear(color color) {
        target_->Clear(D2D1::ColorF(color.r, color.g, color.b));
    }

    void push_clip_rect(rect const& r) {
        target_->PushAxisAlignedClip(make_rect(r), D2D1_ANTIALIAS_MODE_ALIASED);
    }

    void pop_clip_rect() {
        target_->PopAxisAlignedClip();
    }

    void fill_rect(rect const& r, brush const& b) {
        target_->FillRectangle(
            make_rect(r),
            ((brush_impl&)b).brush
        );
    }

    void draw_rect(rect const& r, brush const& b, float width) {
        target_->DrawRectangle(
            make_rect(r),
            ((brush_impl&)b).brush,
            width, nullptr
        );
    }

    void draw_text(rect const& r, utf8string const& text) {
        auto const t = convert.from_bytes(text);

        target_->DrawTextW(
            t.c_str(), t.size(),
            text_format_,
            make_rect(r),
            solid_brush_.brush
        );
    }
    
    //void set_transformation(renderer::matrix const& m) {
    //    auto const mat = reinterpret_cast<D2D1_MATRIX_3X2_F const*>(m.data());
    //    target_->SetTransform(mat);
    //}

    void translate(float x, float y) {
        D2D1_MATRIX_3X2_F m;
        target_->GetTransform(&m);

        m = D2D1::Matrix3x2F::Translation(x, y) * m;

        target_->SetTransform(m);
    }

    std::unique_ptr<g2d::solid_color_brush>
    create_soild_brush(color color) {
        auto brush = win::make_com_ptr([&](ID2D1SolidColorBrush** out) {
            return target_->CreateSolidColorBrush(D2D1::ColorF(color.r, color.g, color.b, color.a), out);
        });

        return std::make_unique<solid_color_brush_impl>(std::move(brush));
    }

    void draw_texture(rect src, rect dest) {
        target_->DrawBitmap(
            texture_,
            make_rect(dest),
            1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
            make_rect(src)
        );
    }

    void create_texture(unsigned w, unsigned h, void const* data) {
        texture_ = win::make_com_ptr([&](ID2D1Bitmap** out) {
            return target_->CreateBitmap(
                D2D1::SizeU(w, h),
                data,
                w * 4,
                D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE)),
                out
            );
        });
    }

    utf8_16_converter convert;

    win::com_ptr<ID2D1Factory>          factory_;
    win::com_ptr<ID2D1HwndRenderTarget> target_;

    win::com_ptr<IDWriteFactory>        write_factory_;
    win::com_ptr<IDWriteTextFormat>     text_format_;

    win::com_ptr<ID2D1Bitmap> texture_;

    solid_color_brush_impl  solid_brush_;
};

