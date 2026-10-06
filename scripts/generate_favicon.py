import json, base64, zlib, struct, os

def create_png(width, height, rgba_rows):
    raw = b''.join(b'\x00' + row for row in rgba_rows)
    compressed = zlib.compress(raw, level=9)
    def chunk(tag, data):
        return struct.pack('>I', len(data)) + tag + data + struct.pack('>I', zlib.crc32(tag + data) & 0xffffffff)
    png = b'\x89PNG\r\n\x1a\n'
    png += chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, 0))
    png += chunk(b'IDAT', compressed)
    png += chunk(b'IEND', b'')
    return png

def create_ico(png_bytes_list):
    num_images = len(png_bytes_list)
    header = struct.pack('<HHH', 0, 1, num_images)
    offset = 6 + 16 * num_images
    dir_entries = b''
    data_blobs = b''
    for width, height, png_data in png_bytes_list:
        w_byte = width if width < 256 else 0
        h_byte = height if height < 256 else 0
        size = len(png_data)
        dir_entries += struct.pack('<BBBBHHII', w_byte, h_byte, 0, 0, 1, 32, size, offset)
        data_blobs += png_data
        offset += size
    return header + dir_entries + data_blobs

def extract_and_generate():
    with open('web/js/animations.js', 'r', encoding='utf-8') as f:
        text = f.read()
    data = json.loads(text[text.find('{'):text.rfind('}')+1])
    raw = base64.b64decode(data['IDLE']['framesB64'][0])

    oled_w, oled_h = 128, 64
    pixels = set()
    for x in range(oled_w):
        for y in range(oled_h):
            srcIndex = x + (y // 8) * oled_w
            srcBit = 1 << (y & 7)
            if (raw[srcIndex] & srcBit) != 0:
                pixels.add((x, y))

    min_x = min(p[0] for p in pixels)
    max_x = max(p[0] for p in pixels)
    min_y = min(p[1] for p in pixels)
    max_y = max(p[1] for p in pixels)
    pw = max_x - min_x + 1  # 35
    ph = max_y - min_y + 1  # 42

    def render_icon(target_size):
        # Scale penguin to occupy ~70% of target size (un poco menos de zoom, cabeza con margen)
        target_ph = max(10, round(target_size * 0.70))
        target_pw = round(target_ph * (pw / ph))
        
        offset_x = (target_size - target_pw) / 2.0
        offset_y = (target_size - target_ph) / 2.0
        
        center = target_size / 2.0
        radius = (target_size / 2.0) - 0.75
        
        # Supersampling 4x4 per pixel for clean anti-aliasing
        sub = 4
        
        rows = []
        for y in range(target_size):
            row = bytearray()
            for x in range(target_size):
                p_count = 0
                d_count = 0
                
                for sy in range(sub):
                    fy = y + (sy + 0.5) / sub
                    dy = fy - center
                    for sx in range(sub):
                        fx = x + (sx + 0.5) / sub
                        dx = fx - center
                        dist = (dx*dx + dy*dy) ** 0.5
                        
                        if dist <= radius:
                            d_count += 1
                            sp_x = (fx - offset_x) / target_pw * pw
                            sp_y = (fy - offset_y) / target_ph * ph
                            ix = int(sp_x)
                            iy = int(sp_y)
                            if 0 <= ix < pw and 0 <= iy < ph and (min_x + ix, min_y + iy) in pixels:
                                p_count += 1
                                
                disc_alpha = d_count / (sub * sub)
                penguin_alpha = p_count / (sub * sub)
                
                if disc_alpha <= 0.01:
                    row.extend([0, 0, 0, 0])
                else:
                    # Negativo monocromático: disco blanco (#ffffff), pingüino negro (#0a0c10)
                    # Sutil borde exterior (#c8ceda) para delimitar en pestañas claras del navegador
                    outer_ratio = dist / radius if radius > 0 else 0
                    if outer_ratio > 0.94:
                        edge_t = min(1.0, (outer_ratio - 0.94) / 0.06)
                        base_r = int(255 * (1 - edge_t) + 200 * edge_t)
                        base_g = int(255 * (1 - edge_t) + 206 * edge_t)
                        base_b = int(255 * (1 - edge_t) + 218 * edge_t)
                    else:
                        base_r, base_g, base_b = 255, 255, 255

                    fg_r, fg_g, fg_b = 10, 12, 16
                    out_r = int(base_r * (1 - penguin_alpha) + fg_r * penguin_alpha)
                    out_g = int(base_g * (1 - penguin_alpha) + fg_g * penguin_alpha)
                    out_b = int(base_b * (1 - penguin_alpha) + fg_b * penguin_alpha)
                    out_a = int(255 * disc_alpha)

                    row.extend([out_r, out_g, out_b, out_a])
            rows.append(bytes(row))
        return create_png(target_size, target_size, rows)

    png_16 = render_icon(16)
    png_32 = render_icon(32)
    png_48 = render_icon(48)
    png_64 = render_icon(64)
    png_128 = render_icon(128)

    os.makedirs('web/assets', exist_ok=True)
    with open('web/assets/favicon.png', 'wb') as f:
        f.write(png_128)
    with open('web/assets/favicon-32x32.png', 'wb') as f:
        f.write(png_32)

    ico_data = create_ico([(16, 16, png_16), (32, 32, png_32), (48, 48, png_48), (64, 64, png_64)])
    with open('web/favicon.ico', 'wb') as f:
        f.write(ico_data)

    print('B&W Favicons generated successfully!')

if __name__ == '__main__':
    extract_and_generate()
