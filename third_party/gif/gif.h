// gif.h -- minimal single-header GIF89a encoder for animated GIFs
// Public domain / CC0.  No warranty.
//
// Usage:
//   GifWriter gw;
//   gif_begin(&gw, "out.gif", width, height, delay_cs);
//   for each frame:
//       gif_write_frame(&gw, rgba_data, width, height, delay_cs);
//   gif_end(&gw);
//
// Each frame is RGBA (4 bytes per pixel, row-major).  The encoder
// builds a per-frame 256-colour palette via median-cut quantisation
// and writes LZW-compressed image data per the GIF89a spec.

#ifndef GIF_H_INCLUDED
#define GIF_H_INCLUDED

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <vector>

struct GifWriter {
    std::FILE* fp  = nullptr;
    int        w   = 0;
    int        h   = 0;
    bool       first_frame = true;
};

namespace gif_detail {

struct PalEntry { uint8_t r, g, b; };

// Median-cut colour quantisation (simple, fast, decent quality).
inline void quantise(const uint8_t* rgba, int count,
                     PalEntry palette[256], uint8_t* indices) {
    struct Box {
        int start, count;
        uint8_t rmin, rmax, gmin, gmax, bmin, bmax;
    };

    struct Pixel { uint8_t r, g, b; int orig_idx; };
    std::vector<Pixel> pixels;
    pixels.reserve(count);
    for (int i = 0; i < count; ++i)
        pixels.push_back({rgba[i*4], rgba[i*4+1], rgba[i*4+2], i});

    auto compute_box = [&](Box& box) {
        box.rmin = box.gmin = box.bmin = 255;
        box.rmax = box.gmax = box.bmax = 0;
        for (int i = box.start; i < box.start + box.count; ++i) {
            auto& p = pixels[i];
            box.rmin = std::min(box.rmin, p.r); box.rmax = std::max(box.rmax, p.r);
            box.gmin = std::min(box.gmin, p.g); box.gmax = std::max(box.gmax, p.g);
            box.bmin = std::min(box.bmin, p.b); box.bmax = std::max(box.bmax, p.b);
        }
    };

    std::vector<Box> boxes;
    boxes.push_back({0, count, 0,0,0,0,0,0});
    compute_box(boxes[0]);

    while (boxes.size() < 256) {
        int best = -1;
        int best_range = 0;
        for (int i = 0; i < static_cast<int>(boxes.size()); ++i) {
            if (boxes[i].count < 2) continue;
            int rr = boxes[i].rmax - boxes[i].rmin;
            int gr = boxes[i].gmax - boxes[i].gmin;
            int br = boxes[i].bmax - boxes[i].bmin;
            int mx = std::max({rr, gr, br});
            if (mx > best_range) { best_range = mx; best = i; }
        }
        if (best < 0) break;

        auto& b = boxes[best];
        int rr = b.rmax - b.rmin;
        int gr = b.gmax - b.gmin;
        int br = b.bmax - b.bmin;

        auto pbegin = pixels.begin() + b.start;
        auto pend   = pbegin + b.count;
        if (rr >= gr && rr >= br)
            std::sort(pbegin, pend, [](auto& a, auto& b){ return a.r < b.r; });
        else if (gr >= br)
            std::sort(pbegin, pend, [](auto& a, auto& b){ return a.g < b.g; });
        else
            std::sort(pbegin, pend, [](auto& a, auto& b){ return a.b < b.b; });

        int half = b.count / 2;
        Box b1{b.start, half, 0,0,0,0,0,0};
        Box b2{b.start + half, b.count - half, 0,0,0,0,0,0};
        compute_box(b1);
        compute_box(b2);
        boxes[best] = b1;
        boxes.push_back(b2);
    }

    // Build palette from box averages
    for (int i = 0; i < static_cast<int>(boxes.size()); ++i) {
        long rsum = 0, gsum = 0, bsum = 0;
        for (int j = boxes[i].start; j < boxes[i].start + boxes[i].count; ++j) {
            rsum += pixels[j].r; gsum += pixels[j].g; bsum += pixels[j].b;
        }
        int n = boxes[i].count;
        palette[i] = {static_cast<uint8_t>(rsum/n),
                       static_cast<uint8_t>(gsum/n),
                       static_cast<uint8_t>(bsum/n)};
    }
    for (int i = static_cast<int>(boxes.size()); i < 256; ++i)
        palette[i] = {0, 0, 0};

    // Map each original pixel to the nearest palette entry
    // First, assign pixels in each box to their box's palette index
    std::vector<uint8_t> idx(count, 0);
    for (int i = 0; i < static_cast<int>(boxes.size()); ++i)
        for (int j = boxes[i].start; j < boxes[i].start + boxes[i].count; ++j)
            idx[pixels[j].orig_idx] = static_cast<uint8_t>(i);

    std::memcpy(indices, idx.data(), count);
}

// LZW compressor for GIF
struct LzwState {
    std::FILE* fp;
    int min_code_size;
    int clear_code;
    int eof_code;
    int code_size;
    int next_code;
    int cur_bits;
    uint32_t cur_byte;
    uint8_t buf[256];
    int buf_len;

    struct DictEntry { int prev; uint8_t c; int code; };
    std::vector<DictEntry> dict;

    void init(std::FILE* f, int bpp) {
        fp = f;
        min_code_size = std::max(bpp, 2);
        clear_code = 1 << min_code_size;
        eof_code = clear_code + 1;
        code_size = min_code_size + 1;
        next_code = eof_code + 1;
        cur_bits = 0;
        cur_byte = 0;
        buf_len = 0;
        dict.clear();
        dict.resize(4096);
        for (int i = 0; i < 4096; ++i) dict[i] = {-1, 0, -1};
    }

    void emit(int code) {
        cur_byte |= static_cast<uint32_t>(code) << cur_bits;
        cur_bits += code_size;
        while (cur_bits >= 8) {
            buf[buf_len++] = static_cast<uint8_t>(cur_byte & 0xFF);
            cur_byte >>= 8;
            cur_bits -= 8;
            if (buf_len >= 255) flush_buf();
        }
    }

    void flush_buf() {
        if (buf_len > 0) {
            std::fputc(buf_len, fp);
            std::fwrite(buf, 1, buf_len, fp);
            buf_len = 0;
        }
    }

    void reset() {
        for (int i = 0; i < 4096; ++i) dict[i] = {-1, 0, -1};
        next_code = eof_code + 1;
        code_size = min_code_size + 1;
    }

    int lookup(int prev, uint8_t c) const {
        for (int i = eof_code + 1; i < next_code; ++i)
            if (dict[i].prev == prev && dict[i].c == c) return i;
        return -1;
    }
};

inline void lzw_encode(std::FILE* fp, const uint8_t* data, int count, int bpp) {
    LzwState st;
    st.init(fp, bpp);

    std::fputc(st.min_code_size, fp);
    st.emit(st.clear_code);

    int cur = data[0];
    for (int i = 1; i < count; ++i) {
        uint8_t c = data[i];
        int found = st.lookup(cur, c);
        if (found >= 0) {
            cur = found;
        } else {
            st.emit(cur);
            if (st.next_code < 4096) {
                st.dict[st.next_code] = {cur, c, st.next_code};
                st.next_code++;
                if (st.next_code > (1 << st.code_size) && st.code_size < 12)
                    st.code_size++;
            } else {
                st.emit(st.clear_code);
                st.reset();
            }
            cur = c;
        }
    }
    st.emit(cur);
    st.emit(st.eof_code);

    if (st.cur_bits > 0) {
        st.buf[st.buf_len++] = static_cast<uint8_t>(st.cur_byte & 0xFF);
    }
    st.flush_buf();
    std::fputc(0, fp); // block terminator
}

inline void write16(std::FILE* fp, uint16_t v) {
    std::fputc(v & 0xFF, fp);
    std::fputc((v >> 8) & 0xFF, fp);
}

} // namespace gif_detail

inline bool gif_begin(GifWriter* gw, const char* filename, int w, int h,
                      uint16_t delay_cs) {
    (void)delay_cs;
    gw->fp = std::fopen(filename, "wb");
    if (!gw->fp) return false;
    gw->w = w;
    gw->h = h;
    gw->first_frame = true;

    std::fwrite("GIF89a", 1, 6, gw->fp);
    gif_detail::write16(gw->fp, static_cast<uint16_t>(w));
    gif_detail::write16(gw->fp, static_cast<uint16_t>(h));
    // GCT flag=0, color res=7, sort=0, GCT size=0
    std::fputc(0x70, gw->fp); // no global colour table
    std::fputc(0, gw->fp);    // bg colour index
    std::fputc(0, gw->fp);    // pixel aspect ratio

    // Netscape Application Extension for looping
    std::fputc(0x21, gw->fp);
    std::fputc(0xFF, gw->fp);
    std::fputc(11, gw->fp);
    std::fwrite("NETSCAPE2.0", 1, 11, gw->fp);
    std::fputc(3, gw->fp);
    std::fputc(1, gw->fp);
    gif_detail::write16(gw->fp, 0); // loop count 0 = infinite
    std::fputc(0, gw->fp);

    return true;
}

inline bool gif_write_frame(GifWriter* gw, const uint8_t* rgba,
                            int w, int h, uint16_t delay_cs) {
    if (!gw->fp) return false;

    int count = w * h;
    gif_detail::PalEntry palette[256];
    std::vector<uint8_t> indices(count);
    gif_detail::quantise(rgba, count, palette, indices.data());

    // Graphics Control Extension
    std::fputc(0x21, gw->fp);
    std::fputc(0xF9, gw->fp);
    std::fputc(4, gw->fp);
    std::fputc(0x00, gw->fp); // disposal = none, no transparency
    gif_detail::write16(gw->fp, delay_cs);
    std::fputc(0, gw->fp);  // transparent colour index (unused)
    std::fputc(0, gw->fp);  // block terminator

    // Image Descriptor with local colour table
    std::fputc(0x2C, gw->fp);
    gif_detail::write16(gw->fp, 0);
    gif_detail::write16(gw->fp, 0);
    gif_detail::write16(gw->fp, static_cast<uint16_t>(w));
    gif_detail::write16(gw->fp, static_cast<uint16_t>(h));
    // Local colour table flag=1, interlace=0, sort=0, size=7 (256 entries)
    std::fputc(0x87, gw->fp);

    // Local Colour Table (256 * 3 bytes)
    for (int i = 0; i < 256; ++i) {
        std::fputc(palette[i].r, gw->fp);
        std::fputc(palette[i].g, gw->fp);
        std::fputc(palette[i].b, gw->fp);
    }

    gif_detail::lzw_encode(gw->fp, indices.data(), count, 8);

    gw->first_frame = false;
    return true;
}

inline bool gif_end(GifWriter* gw) {
    if (!gw->fp) return false;
    std::fputc(0x3B, gw->fp); // GIF trailer
    std::fclose(gw->fp);
    gw->fp = nullptr;
    return true;
}

#endif // GIF_H_INCLUDED
