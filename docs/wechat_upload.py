#!/usr/bin/env python3
"""
将 user-guide.md 转换为微信公众号格式并上传为草稿。

用法:
    python wechat_upload.py --appid <APPID> --secret <APPSECRET>

流程:
    1. 读取 user-guide.md → 转换为微信公众号兼容的 HTML
    2. 上传本地图片到微信素材库 → 获取 media_id 和 URL
    3. 替换 HTML 中的图片引用
    4. 创建草稿
"""

import argparse
import json
import os
import re
import sys
import tempfile
import time
import io

# 强制 stdout 使用 UTF-8，解决 Windows GBK 编码问题
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')
from pathlib import Path
from urllib.parse import urljoin

import requests
from PIL import Image

# ── 路径配置 ──────────────────────────────────────────────
SCRIPT_DIR = Path(__file__).resolve().parent
HTML_OUTPUT = SCRIPT_DIR / "wechat-output.html"
IMAGE_DIR = Path(r"C:\Users\lhx\AppData\Roaming\Typora\typora-user-images")
CONFIG_FILE = SCRIPT_DIR / "wechat_config.json"

# ── 微信 API ──────────────────────────────────────────────
WECHAT_TOKEN_URL = "https://api.weixin.qq.com/cgi-bin/token"
WECHAT_MATERIAL_URL = "https://api.weixin.qq.com/cgi-bin/material/add_material"
WECHAT_MEDIA_UPLOAD_URL = "https://api.weixin.qq.com/cgi-bin/media/upload"
WECHAT_DRAFT_URL = "https://api.weixin.qq.com/cgi-bin/draft/add"


def get_access_token(appid: str, secret: str) -> str:
    """获取微信公众号 access_token"""
    resp = requests.get(WECHAT_TOKEN_URL, params={
        "grant_type": "client_credential",
        "appid": appid,
        "secret": secret,
    }, timeout=15)
    data = resp.json()
    if "access_token" not in data:
        raise RuntimeError(f"获取 access_token 失败: {data}")
    print(f"✅ access_token 获取成功 (expires_in={data.get('expires_in')}s)")
    return data["access_token"]


def upload_thumb(token: str, image_path: Path) -> str:
    """上传缩略图为永久素材，返回 media_id（用于草稿封面）

    微信 thumb 要求: JPG 格式, ≤64KB"""
    # 缩小到微信封面建议尺寸
    img = Image.open(image_path).convert("RGB")
    thumb_size = (300, int(img.height * 300 / img.width))
    img = img.resize(thumb_size, Image.LANCZOS)

    tmp = tempfile.NamedTemporaryFile(suffix=".jpg", delete=False)
    try:
        for quality in range(85, 9, -5):
            tmp.seek(0)
            img.save(tmp, format="JPEG", quality=quality, optimize=True)
            size = tmp.tell()
            if size <= 60000:
                break
        tmp.close()
        filesize = os.path.getsize(tmp.name)
        print(f"  📸 缩略图压缩: {image_path.name} → {thumb_size[0]}×{thumb_size[1]} JPG {filesize}bytes")

        with open(tmp.name, "rb") as f:
            # 使用永久素材 API（type=thumb），其返回的 media_id 可被 draft/add 接受
            resp = requests.post(
                WECHAT_MATERIAL_URL,
                params={"access_token": token, "type": "thumb"},
                files={"media": ("thumb.jpg", f, "image/jpeg")},
                timeout=30,
            )
    finally:
        os.unlink(tmp.name)

    data = resp.json()
    if "media_id" not in data:
        raise RuntimeError(f"上传缩略图失败 {image_path.name}: {data}")
    print(f"  📌 封面缩略图上传成功: media_id={data['media_id']}")
    return data["media_id"]


def upload_image(token: str, image_path: Path) -> dict:
    """上传图片为永久素材，返回 {media_id, url}"""
    filename = image_path.name
    with open(image_path, "rb") as f:
        # 微信 add_material 接口需要 multipart/form-data
        resp = requests.post(
            WECHAT_MATERIAL_URL,
            params={"access_token": token, "type": "image"},
            files={"media": (filename, f, "image/png")},
            timeout=30,
        )
    data = resp.json()
    if "media_id" not in data:
        raise RuntimeError(f"上传图片失败 {filename}: {data}")
    print(f"  📷 {filename} → media_id={data['media_id']} url={data.get('url', 'N/A')}")
    return {"media_id": data["media_id"], "url": data.get("url", "")}


def markdown_to_wechat_html(md_text: str, image_map: dict) -> str:
    """
    将 Markdown 转换为微信公众号兼容的 HTML。

    微信公众号编辑器的 HTML 限制：
    - 只支持有限的标签: section, p, h1-h6, blockquote, ul, ol, li,
      table, strong, em, span, img, br, div, pre, code
    - 不支持 class，只能用内联 style
    - 不支持 JavaScript
    - 图片需要先上传到微信素材库
    """

    def escape_html(text: str) -> str:
        return text.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")

    def replace_image(match):
        """将 Typora 本地图片路径替换为微信 URL"""
        alt = match.group(1)
        img_path = match.group(2)
        fname = os.path.basename(img_path)
        if fname in image_map:
            return f'<p style="text-align:center;"><img src="{image_map[fname]["url"]}" alt="{alt}" style="max-width:100%;height:auto;"/></p>'
        return match.group(0)

    # 先替换图片
    img_pattern = re.compile(r'!\[([^\]]*)\]\(([^)]+)\)')
    md_text = img_pattern.sub(replace_image, md_text)

    lines = md_text.split("\n")
    html_parts = []
    i = 0

    in_table = False
    in_code_block = False
    in_list = False
    list_type = None  # 'ul' or 'ol'

    CSS_P = 'style="margin:0 0 10px 0;font-size:15px;color:#3f3f3f;line-height:1.75;letter-spacing:0.5px;"'
    CSS_BQ = 'style="margin:12px 0;padding:10px 16px;border-left:4px solid #5b9bd5;background:#f0f7ff;color:#555;font-size:14px;line-height:1.7;border-radius:0 4px 4px 0;"'
    CSS_CODE = 'style="padding:12px 16px;background:#f5f5f5;border-radius:4px;font-size:13px;line-height:1.6;font-family:Consolas,Monaco,monospace;overflow-x:auto;white-space:pre-wrap;word-break:break-all;color:#333;"'
    CSS_H2 = 'style="margin:28px 0 14px 0;padding-left:12px;border-left:4px solid #07c160;font-size:20px;font-weight:bold;color:#000;line-height:1.4;"'
    CSS_H3 = 'style="margin:22px 0 10px 0;font-size:17px;font-weight:bold;color:#000;line-height:1.4;"'
    CSS_TABLE = 'style="width:100%;border-collapse:collapse;margin:10px 0;font-size:14px;color:#3f3f3f;"'
    CSS_TH = 'style="padding:8px 12px;background:#f5f5f5;border:1px solid #e0e0e0;text-align:left;font-weight:bold;color:#000;"'
    CSS_TD = 'style="padding:8px 12px;border:1px solid #e0e0e0;"'
    CSS_HR = '<hr style="margin:24px 0;border:none;border-top:1px solid #e0e0e0;"/>'
    CSS_UL = 'style="padding-left:1.6em;margin:8px 0;list-style-type:disc;"'
    CSS_OL = 'style="padding-left:1.6em;margin:8px 0;"'
    CSS_LI_UL = 'style="margin-bottom:4px;font-size:15px;color:#3f3f3f;line-height:1.75;"'
    CSS_LI_OL = 'style="margin-bottom:4px;font-size:15px;color:#3f3f3f;line-height:1.75;"'

    def _fmt_inline(text: str) -> str:
        """处理行内格式：粗体、代码、链接"""
        t = re.sub(r'\*\*(.+?)\*\*', r'<strong>\1</strong>', text)
        t = re.sub(r'`([^`]+)`', r'<code style="padding:2px 6px;background:#f0f0f0;border-radius:3px;font-size:13px;color:#c7254e;">\1</code>', t)
        t = re.sub(r'\[([^\]]+)\]\(([^)]+)\)', r'<a href="\2" style="color:#5b9bd5;">\1</a>', t)
        return t

    def close_list():
        nonlocal in_list, list_type
        if in_list:
            tag = list_type or "ul"
            html_parts.append(f"</{tag}>")
            in_list = False
            list_type = None

    def close_table():
        nonlocal in_table
        if in_table:
            html_parts.append("</table>")
            in_table = False

    while i < len(lines):
        line = lines[i]

        # 空行处理
        if not line.strip():
            close_list()
            close_table()
            if in_code_block:
                html_parts.append("")
            else:
                html_parts.append("<br/>")
            i += 1
            continue

        # 代码块
        if line.strip().startswith("```"):
            close_list()
            close_table()
            if in_code_block:
                html_parts.append("</code></pre>")
                in_code_block = False
            else:
                html_parts.append(f"<pre {CSS_CODE}><code>")
                in_code_block = True
            i += 1
            continue

        if in_code_block:
            html_parts.append(escape_html(line))
            i += 1
            continue

        # 标题
        if line.startswith("# "):
            close_list()
            close_table()
            title = line[2:].strip()
            html_parts.append(f'<h1 style="text-align:center;font-size:24px;font-weight:bold;color:#000;margin:20px 0 20px 0;line-height:1.4;">{title}</h1>')
            i += 1
            continue

        if line.startswith("## "):
            close_list()
            close_table()
            title = line[3:].strip()
            html_parts.append(f"<h2 {CSS_H2}>{title}</h2>")
            i += 1
            continue

        if line.startswith("### "):
            close_list()
            close_table()
            title = line[4:].strip()
            html_parts.append(f"<h3 {CSS_H3}>{title}</h3>")
            i += 1
            continue

        # 分隔线
        if line.strip() == "---":
            close_list()
            close_table()
            html_parts.append(CSS_HR)
            i += 1
            continue

        # 引用块（支持内部嵌套有序/无序列表）
        if line.strip().startswith("> "):
            close_list()
            close_table()
            raw_quote_lines = []
            while i < len(lines) and lines[i].strip().startswith("> "):
                raw_quote_lines.append(lines[i].strip()[2:])
                i += 1

            # 解析引用块内部：将连续行分组，检测并生成嵌套列表
            quote_parts = []
            qj = 0
            while qj < len(raw_quote_lines):
                ql = raw_quote_lines[qj]

                # 空行 → 段落分隔
                if not ql.strip():
                    qj += 1
                    continue

                # 检测引用内的无序列表项
                qul = re.match(r'^[-*]\s+(.*)', ql)
                if qul:
                    quote_parts.append(f"<ul {CSS_UL}>")
                    while qj < len(raw_quote_lines):
                        m = re.match(r'^[-*]\s+(.*)', raw_quote_lines[qj])
                        if not m:
                            break
                        item = _fmt_inline(m.group(1))
                        quote_parts.append(f'<li {CSS_LI_UL}>{item}</li>')
                        qj += 1
                    quote_parts.append("</ul>")
                    continue

                # 检测引用内的有序列表项
                qol = re.match(r'^\d+\.\s+(.*)', ql)
                if qol:
                    quote_parts.append(f"<ol {CSS_OL}>")
                    while qj < len(raw_quote_lines):
                        m = re.match(r'^\d+\.\s+(.*)', raw_quote_lines[qj])
                        if not m:
                            break
                        item = _fmt_inline(m.group(1))
                        quote_parts.append(f'<li {CSS_LI_OL}>{item}</li>')
                        qj += 1
                    quote_parts.append("</ol>")
                    continue

                # 普通引用段落
                para = _fmt_inline(ql)
                quote_parts.append(f'<p style="margin:0 0 6px 0;font-size:14px;color:#555;line-height:1.7;">{para}</p>')
                qj += 1

            content = "".join(quote_parts)
            html_parts.append(f"<blockquote {CSS_BQ}>{content}</blockquote>")
            continue

        # 列表项
        ul_match = re.match(r'^(\s*)[-*]\s+(.*)', line)
        ol_match = re.match(r'^(\s*)\d+\.\s+(.*)', line)

        if ul_match and not line.strip().startswith("**"):
            if not in_list or list_type != "ul":
                close_list()
                html_parts.append(f"<ul {CSS_UL}>")
                in_list = True
                list_type = "ul"
            item = _fmt_inline(ul_match.group(2))
            html_parts.append(f'<li {CSS_LI_UL}>{item}</li>')
            i += 1
            continue

        if ol_match:
            if not in_list or list_type != "ol":
                close_list()
                html_parts.append(f"<ol {CSS_OL}>")
                in_list = True
                list_type = "ol"
            item = _fmt_inline(ol_match.group(2))
            html_parts.append(f'<li {CSS_LI_OL}>{item}</li>')
            i += 1
            continue

        # 表格（检测 | 开头的行）
        if line.strip().startswith("|") and line.strip().endswith("|"):
            close_list()
            if not in_table:
                html_parts.append(f"<table {CSS_TABLE}>")
                in_table = True

            # 跳过分隔行 (|---|---|)
            if re.match(r'^\|[\s\-:|]+\|$', line.strip()):
                i += 1
                continue

            cells = [c.strip() for c in line.strip().split("|")[1:-1]]
            # 判断是否为表头（下一行是分隔线，或者这是表格第一行）
            is_header = (i + 1 < len(lines) and
                         re.match(r'^\|[\s\-:|]+\|$', lines[i + 1].strip()))

            tag = "th" if is_header else "td"
            css = CSS_TH if is_header else CSS_TD
            cell_html = "".join(f"<{tag} {css}>{cell}</{tag}>" for cell in cells)
            html_parts.append(f"<tr>{cell_html}</tr>")
            i += 1
            continue

        # 普通段落
        close_list()
        close_table()

        para = _fmt_inline(line.strip())
        html_parts.append(f"<p {CSS_P}>{para}</p>")
        i += 1

    # 关闭未闭合的块
    close_list()
    close_table()
    if in_code_block:
        html_parts.append("</code></pre>")

    # 组装完整 HTML（微信公众号用 section 包裹）
    body = "\n".join(html_parts)

    full_html = f"""\
<section style="max-width:677px;margin:0 auto;padding:10px 0;">
{body}
</section>
<section style="max-width:677px;margin:30px auto 10px;padding:16px;background:#f9f9f9;border-radius:6px;text-align:center;font-size:13px;color:#999;">
  <p style="margin:0;">🔗 项目地址：<a href="https://github.com/damon-liu/Dji-cloud-api-tool" style="color:#5b9bd5;">github.com/damon-liu/Dji-cloud-api-tool</a></p>
  <p style="margin:4px 0 0 0;">如果这个项目对你有帮助，请点赞👍 + 关注👀 + Star⭐！</p>
</section>"""
    return full_html


def create_draft(token: str, title: str, content_html: str, thumb_media_id: str = ""):
    """创建微信公众号草稿"""
    articles = [{
        "title": title,
        "content": content_html,
        "content_source_url": "https://github.com/damon-liu/Dji-cloud-api-tool",
        "need_open_comment": 0,
        "only_fans_can_comment": 0,
    }]
    if thumb_media_id:
        articles[0]["thumb_media_id"] = thumb_media_id

    body = {"articles": articles}

    print(f"   title: {title} (len={len(title)})")
    print(f"   content length: {len(content_html)} chars")
    print(f"   thumb_media_id: {thumb_media_id}")

    # 必须显式指定 ensure_ascii=False 并以 UTF-8 字节发送，
    # 否则 requests 的 json= 参数会将中文转成 \uXXXX 导致微信端乱码
    json_bytes = json.dumps(body, ensure_ascii=False).encode("utf-8")
    resp = requests.post(
        WECHAT_DRAFT_URL,
        params={"access_token": token},
        data=json_bytes,
        headers={"Content-Type": "application/json; charset=utf-8"},
        timeout=30,
    )
    data = resp.json()
    print(f"   response: {json.dumps(data, ensure_ascii=False)}")
    if "media_id" not in data:
        raise RuntimeError(f"创建草稿失败: {data}")
    print(f"✅ 草稿创建成功！media_id={data['media_id']}")
    return data["media_id"]


def main():
    parser = argparse.ArgumentParser(description="上传 Markdown 文章到微信公众号草稿箱")
    parser.add_argument("--appid", default=None, help="微信公众号 AppID（可省略，从 wechat_config.json 读取）")
    parser.add_argument("--secret", default=None, help="微信公众号 AppSecret（可省略，从 wechat_config.json 读取）")
    parser.add_argument("--file", default=None, help="Markdown 文件路径（默认: user-guide.md）")
    parser.add_argument("--title", default=None, help="文章标题（默认: 从 Markdown 第一个 H1 提取）")
    args = parser.parse_args()

    # 优先命令行，其次配置文件
    appid = args.appid
    secret = args.secret

    if (not appid or not secret) and CONFIG_FILE.exists():
        cfg = json.loads(CONFIG_FILE.read_text(encoding="utf-8"))
        if not appid:
            appid = cfg.get("appid", "")
        if not secret:
            secret = cfg.get("appsecret", "")
        if appid and secret:
            print(f"📋 从 {CONFIG_FILE} 读取凭证")

    if not appid or not secret:
        sys.exit("❌ 请提供 --appid 和 --secret，或在 wechat_config.json 中配置。")

    # 确定输入文件
    if args.file:
        md_file = Path(args.file)
    else:
        md_file = SCRIPT_DIR / "user-guide.md"

    # 1. 获取 access_token
    print("🔑 获取 access_token...")
    token = get_access_token(appid, secret)

    # 2. 读取 Markdown
    print(f"\n📄 读取 Markdown: {md_file}")
    md_text = md_file.read_text(encoding="utf-8")

    # 2b. 提取标题（第一个 H1）
    title = args.title
    if not title:
        m = re.search(r'^#\s+(.+)', md_text, re.MULTILINE)
        title = m.group(1).strip() if m else "未命名文章"

    # 3. 找出所有本地图片
    local_images = set()
    for m in re.finditer(r'!\[([^\]]*)\]\(([^)]+)\)', md_text):
        img_path = m.group(2)
        fname = os.path.basename(img_path)
        local_path = IMAGE_DIR / fname
        if local_path.exists():
            local_images.add((fname, local_path))

    print(f"\n📷 找到 {len(local_images)} 张本地图片，开始上传...")

    # 4a. 先上传缩略图（临时素材，用于草稿封面）
    thumb_media_id = ""
    if local_images:
        first_fname, first_path = sorted(local_images)[0]
        thumb_media_id = upload_thumb(token, first_path)

    # 4b. 上传图片到微信永久素材库（用于文章内嵌）
    image_map = {}
    for fname, local_path in sorted(local_images):
        result = upload_image(token, local_path)
        image_map[fname] = result
        # 微信 API 有频率限制，加点延迟
        time.sleep(0.3)

    # 5. 转换 Markdown → 微信公众号 HTML
    print(f"\n🔄 转换 Markdown → 微信公众号 HTML...")
    html = markdown_to_wechat_html(md_text, image_map)

    # 6. 保存转换后的 HTML（供检查用）
    HTML_OUTPUT.write_text(html, encoding="utf-8")
    print(f"💾 HTML 已保存到: {HTML_OUTPUT}")

    # 7. 创建草稿
    print(f"\n📝 创建草稿...")
    draft_id = create_draft(token, title, html, thumb_media_id)

    print(f"\n🎉 全部完成！")
    print(f"   草稿 media_id: {draft_id}")
    print(f"   请在微信公众号后台「草稿箱」中查看和编辑。")


if __name__ == "__main__":
    main()
