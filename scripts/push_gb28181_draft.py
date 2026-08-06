#!/usr/bin/env python3
"""
Push GB28181 WeChat article to draft box.
Handles: Mermaid → PNG via mermaid.ink, base64 image extraction,
WeChat CDN upload, HTML replacement, draft push.
"""

import json, os, re, sys, ssl, base64, urllib.request, urllib.error, tempfile, hashlib

# ── Config ──────────────────────────────────────────────
APPID = "wx10db5c92106d9a30"
APPSECRET = "b1042249e1c1f9a32fd7c46650811c93"

PROJECT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
HTML_FILE = os.path.join(PROJECT, "docs", "articles", "GB28181国标推流实战分析-wechat.html")
MD_FILE   = os.path.join(PROJECT, "docs", "articles", "GB28181国标推流实战分析.md")

TITLE = "逐帧剖析 GB28181 推流：信令协商与媒体传输全解"
AUTHOR = "damon.liu"
DIGEST = ("基于实际网络抓包数据，逐帧分析GB28181国标推流的完整信令交互过程，"
          "涵盖SIP注册、MANSCDP保活、INVITE/SDP协商、RTP/PS媒体传输与BYE拆除等关键环节。")

# ── API helper ──────────────────────────────────────────

def api(path, data=None, files=None):
    ctx = ssl.create_default_context()
    url = f"https://api.weixin.qq.com{path}"
    if files:
        boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW"
        body = b""
        for name, (filename, content, ct) in files.items():
            body += f"--{boundary}\r\n".encode()
            body += f'Content-Disposition: form-data; name="{name}"; filename="{filename}"\r\n'.encode()
            body += f"Content-Type: {ct}\r\n\r\n".encode()
            body += content + b"\r\n"
        body += f"--{boundary}--\r\n".encode()
        req = urllib.request.Request(url, data=body)
        req.add_header("Content-Type", f"multipart/form-data; boundary={boundary}")
    elif data:
        req = urllib.request.Request(url, data=json.dumps(data, ensure_ascii=False).encode("utf-8"))
        req.add_header("Content-Type", "application/json; charset=utf-8")
    else:
        req = urllib.request.Request(url)
    try:
        with urllib.request.urlopen(req, timeout=30, context=ctx) as resp:
            return json.loads(resp.read().decode("utf-8"))
    except urllib.error.HTTPError as e:
        return json.loads(e.read().decode("utf-8"))


def get_token():
    r = api(f"/cgi-bin/token?grant_type=client_credential&appid={APPID}&secret={APPSECRET}")
    if "access_token" in r:
        return r["access_token"]
    print(f"[ERROR] Token: {r}")
    sys.exit(1)


def upload_img(token, data_bytes, filename="image.png"):
    """Upload image to WeChat CDN. Returns URL string or None."""
    ext = filename.rsplit(".", 1)[-1].lower()
    mime = {"jpg": "image/jpeg", "jpeg": "image/jpeg",
            "png": "image/png", "gif": "image/gif"}.get(ext, "image/png")
    if not filename.startswith("."):
        filename = f"img.{ext}"
    r = api(f"/cgi-bin/media/uploadimg?access_token={token}",
            files={"media": (filename, data_bytes, mime)})
    return r.get("url")


def upload_cover(token, data_bytes, filename="cover.png"):
    """Upload permanent material for cover. Returns media_id or None."""
    ext = filename.rsplit(".", 1)[-1].lower()
    mime = {"jpg": "image/jpeg", "jpeg": "image/jpeg",
            "png": "image/png"}.get(ext, "image/png")
    r = api(f"/cgi-bin/material/add_material?access_token={token}&type=image",
            files={"media": (filename, data_bytes, mime)})
    return r.get("media_id")


# ── Mermaid → PNG via kroki.io / mermaid.ink ──────────

def _fetch_mermaid_png(url, description):
    """Try fetching a Mermaid render URL with browser-like headers."""
    ctx = ssl.create_default_context()
    req = urllib.request.Request(url, headers={
        "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
                       "(KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36",
        "Accept": "image/png,image/*",
    })
    try:
        with urllib.request.urlopen(req, timeout=30, context=ctx) as resp:
            data = resp.read()
            # Validate it's a PNG (starts with \x89PNG)
            if data[:8] == b'\x89PNG\r\n\x1a\n':
                return data
            print(f"      {description}: not a valid PNG ({len(data)} bytes, header={data[:4]!r})")
            return None
    except Exception as e:
        print(f"      {description}: {e}")
        return None


def mermaid_to_png(diagram_text):
    """Render a Mermaid diagram to PNG via kroki.io (primary) or mermaid.ink (fallback)."""
    encoded = base64.urlsafe_b64encode(diagram_text.encode("utf-8")).decode("ascii").rstrip("=")
    print(f"      Rendering Mermaid diagram ({len(diagram_text)} chars)...")

    # Primary: kroki.io (supports Mermaid → PNG)
    result = _fetch_mermaid_png(
        f"https://kroki.io/mermaid/png/{encoded}",
        "kroki.io"
    )
    if result:
        return result

    # Fallback: mermaid.ink
    result = _fetch_mermaid_png(
        f"https://mermaid.ink/img/{encoded}?type=png",
        "mermaid.ink"
    )
    if result:
        return result

    return None


# ── Extract Mermaid diagrams from markdown ─────────────

def extract_mermaids(md_path):
    """Extract all ```mermaid blocks from markdown. Returns list of diagram texts."""
    with open(md_path, "r", encoding="utf-8") as f:
        content = f.read()
    diagrams = []
    pattern = re.compile(r'```mermaid\n(.*?)```', re.DOTALL)
    for m in pattern.finditer(content):
        diagrams.append(m.group(1).strip())
    return diagrams


# ── Extract base64 images from HTML ─────────────────────

def extract_base64_images(html):
    """Find all data:image base64 strings in HTML. Returns list of (old_src, image_bytes, ext)."""
    pattern = re.compile(r'<img[^>]*src="(data:image/(png|jpeg|jpg|gif);base64,([^"]+))"')
    results = []
    for m in pattern.finditer(html):
        full_src = m.group(1)
        mime_type = m.group(2)
        b64_data = m.group(3)
        try:
            img_bytes = base64.b64decode(b64_data)
        except Exception:
            continue
        ext = mime_type if mime_type != "jpeg" else "jpg"
        results.append((full_src, img_bytes, ext))
    return results


# ── Main ────────────────────────────────────────────────

def main():
    print("=" * 56)
    print("Push GB28181 Article to WeChat Draft Box")
    print("=" * 56)

    # ── 1. Token ──
    print("\n[1/5] Get access token...")
    token = get_token()
    print(f"      OK: {token[:20]}...")

    # ── 1.5 Mermaid diagrams → PNG ──
    print("\n[1.5/5] Render Mermaid diagrams to PNG...")
    mermaids = extract_mermaids(MD_FILE)
    print(f"      Found {len(mermaids)} Mermaid diagrams")
    mermaid_pngs = []
    for idx, diagram in enumerate(mermaids):
        png = mermaid_to_png(diagram)
        if png:
            fname = f"gb28181_mermaid_{idx}.png"
            mermaid_pngs.append((fname, png))
            tmp = os.path.join(tempfile.gettempdir(), fname)
            with open(tmp, "wb") as f:
                f.write(png)
            print(f"      Saved -> {tmp} ({len(png)} bytes)")
        else:
            print(f"      FAILED for diagram {idx}")
            mermaid_pngs.append(None)

    # ── 2. Load HTML ──
    print("\n[2/5] Load HTML and extract images...")
    with open(HTML_FILE, "r", encoding="utf-8") as f:
        html = f.read()
    print(f"      HTML size: {len(html):,} chars")

    # Update title in HTML
    html = html.replace(
        "<title>逐帧剖析 GB28181 推流</title>",
        f"<title>{TITLE}</title>"
    )

    # ── 3. Process images ──
    print("\n[3/5] Upload images to WeChat CDN...")

    # 3a. Extract base64 images from HTML
    base64_imgs = extract_base64_images(html)
    print(f"      Found {len(base64_imgs)} base64 embedded image(s)")
    for old_src, img_bytes, ext in base64_imgs:
        fname = f"gb28181_screenshot.{ext}"
        print(f"      Upload screenshot ({len(img_bytes)} bytes)...")
        wx_url = upload_img(token, img_bytes, fname)
        if wx_url:
            html = html.replace(old_src, wx_url)
            print(f"      -> {wx_url[:70]}...")
        else:
            print(f"      FAILED to upload screenshot")

    # 3b. Upload Mermaid PNGs and replace in HTML
    # Match: <div ...><pre class="mermaid" ...>...</pre><p ...>...</p></div>
    mermaid_pattern = re.compile(
        r'<div\s[^>]*>\s*<pre\s+class="mermaid"[^>]*>.*?</pre>\s*<p[^>]*>.*?</p>\s*</div>',
        re.DOTALL
    )
    mermaid_blocks = mermaid_pattern.findall(html)
    print(f"      Found {len(mermaid_blocks)} Mermaid block(s) in HTML")

    for idx, (block, png_data) in enumerate(zip(mermaid_blocks, mermaid_pngs)):
        if png_data is None:
            print(f"      SKIP Mermaid block {idx} (no PNG)")
            continue
        fname, png_bytes = png_data
        print(f"      Upload Mermaid diagram {idx} ({len(png_bytes)} bytes)...")
        wx_url = upload_img(token, png_bytes, fname)
        if wx_url:
            # Replace the entire mermaid container with a simple img
            replacement = (
                f'<div style="text-align:center;margin:16px 4px;">'
                f'<img style="max-width:100%;display:block;margin:14px auto;border-radius:4px;" '
                f'src="{wx_url}" alt="流程图 {idx + 1}">'
                f'<p style="font-size:12px;color:#999;margin-top:4px;text-align:center;">'
                f'▲ 图{idx + 1}</p></div>'
            )
            html = html.replace(block, replacement, 1)
            print(f"      -> {wx_url[:70]}...")
        else:
            print(f"      FAILED to upload Mermaid diagram {idx}")

    # 3c. Check for any remaining local images or base64
    remaining_b64 = re.findall(r'data:image/[^"]+', html)
    if remaining_b64:
        print(f"      WARN: {len(remaining_b64)} base64 image(s) still in HTML")

    # ── 4. Cover image ──
    print("\n[4/5] Upload cover image...")
    cover_id = None
    # Use first screenshot (or Mermaid) as cover
    cover_bytes = None
    cover_ext = "png"
    if base64_imgs:
        cover_bytes = base64_imgs[0][1]
        cover_ext = base64_imgs[0][2]
    elif mermaid_pngs and mermaid_pngs[0]:
        cover_bytes = mermaid_pngs[0][1]

    if cover_bytes:
        cover_id = upload_cover(token, cover_bytes, f"gb28181_cover.{cover_ext}")
        print(f"      media_id: {cover_id}" if cover_id else f"      FAILED")
    else:
        print(f"      SKIP (no cover image available)")

    # ── 5. Push draft ──
    print("\n[5/5] Push draft...")

    article = {
        "title": TITLE,
        "author": AUTHOR,
        "digest": DIGEST,
        "content": html,
        "content_source_url": "",
        "need_open_comment": 0,
        "only_fans_can_comment": 0,
    }
    if cover_id:
        article["thumb_media_id"] = cover_id

    r = api(f"/cgi-bin/draft/add?access_token={token}", data={"articles": [article]})

    if "media_id" in r:
        print(f"      SUCCESS! media_id: {r['media_id']}")
        print(f"\nDone. Visit https://mp.weixin.qq.com -> Drafts to review.")
    else:
        errcode = r.get("errcode", "?")
        errmsg = r.get("errmsg", "unknown")
        hints = {
            40001: "token expired / invalid",
            40007: "bad media_id or media type",
            40014: "token invalid",
            41001: "access_token missing",
            42001: "token expired",
            45009: "API rate limit hit",
            50001: "user unauthorized",
            53010: "content format error"
        }
        hint = hints.get(errcode, "")
        print(f"      FAILED: errcode={errcode}, errmsg={errmsg}")
        if hint:
            print(f"      Hint: {hint}")
        # Save debug HTML
        debug_file = os.path.join(PROJECT, "docs", "articles", "gb28181_debug.html")
        with open(debug_file, "w", encoding="utf-8") as f:
            f.write(html)
        print(f"      Debug HTML saved: {debug_file}")


if __name__ == "__main__":
    main()
