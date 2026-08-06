#!/usr/bin/env python3
"""
Generate WeChat-compatible HTML from markdown.
Uses line-by-line <p> approach for code blocks to prevent content squeezing.
"""

import re, os

SRC = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                   "docs", "articles", "GB28181国标推流实战分析.md")
DST = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                   "docs", "articles", "GB28181国标推流实战分析-wechat.html")

CSS = {
    "body": "margin:0;padding:10px 8px 20px;"
            "font-family:-apple-system,BlinkMacSystemFont,'PingFang SC','Hiragino Sans GB',"
            "'Microsoft YaHei','Helvetica Neue',Arial,sans-serif;"
            "font-size:15px;color:#3f3f3f;line-height:1.85;letter-spacing:0.5px;"
            "background-color:#ffffff;",

    "h1": "text-align:center;font-size:22px;font-weight:700;color:#111;"
          "margin:18px 8px 6px;padding-bottom:14px;border-bottom:2px solid #07c160;"
          "line-height:1.4;letter-spacing:1px;",

    "h2": "font-size:18px;font-weight:700;color:#1a1a1a;margin:28px 4px 14px;"
          "padding:6px 0 6px 14px;border-left:4px solid #07c160;"
          "background-color:#f0faf4;line-height:1.5;",

    "h3": "font-size:16px;font-weight:700;color:#333;margin:20px 4px 10px;"
          "padding-left:8px;border-left:3px solid #52c97d;line-height:1.5;",

    "h4": "font-size:15px;font-weight:700;color:#555;margin:14px 4px 8px;",

    "p": "margin:8px 4px;text-align:justify;",

    "table": "width:100%;border-collapse:collapse;margin:12px 0;font-size:13px;"
             "box-shadow:0 1px 3px rgba(0,0,0,.08);border-radius:4px;",

    "th": "background-color:#07c160;color:#fff;padding:9px 10px;text-align:left;"
           "font-weight:600;font-size:13px;",

    "td": "padding:8px 10px;border-bottom:1px solid #eee;font-size:13px;"
          "background-color:#fff;",

    # Code block wrapper (section)
    "code_wrap": "background-color:#282c34;border-radius:6px;padding:12px 0;"
                 "margin:14px 0;overflow-x:scroll;-webkit-overflow-scrolling:touch;"
                 "border:1px solid #e0e0e0;",

    # Each line in a code block
    "code_line": "margin:0;padding:2px 16px;font-size:13px;line-height:1.7;"
                 "font-family:'SFMono-Regular',Consolas,'Liberation Mono',Menlo,monospace;"
                 "color:#abb2bf;white-space:nowrap;display:block;text-align:left;",

    # Inline code (backticks)
    "icode": "font-family:'SFMono-Regular',Consolas,'Liberation Mono',Menlo,monospace;"
             "background-color:#f0f0f0;padding:1.5px 6px;border-radius:3px;font-size:13px;"
             "color:#c7254e;",

    "bq_warn": "border-left:4px solid #f56c6c;background-color:#fef0f0;padding:10px 14px;"
               "margin:12px 4px;color:#666;font-size:14px;border-radius:0 4px 4px 0;",

    "bq_info": "border-left:4px solid #409eff;background-color:#ecf5ff;padding:10px 14px;"
               "margin:12px 4px;color:#666;font-size:14px;border-radius:0 4px 4px 0;",

    "bq_tip": "border-left:4px solid #07c160;background-color:#f0faf4;padding:10px 14px;"
              "margin:12px 4px;color:#555;font-size:14px;border-radius:0 4px 4px 0;",

    "img": "max-width:100%;display:block;margin:14px auto;border-radius:4px;",

    "ul": "margin:8px 4px;padding-left:22px;",
    "ol": "margin:8px 4px;padding-left:22px;",
    "li": "margin:4px 0;font-size:14px;",

    "strong": "color:#1a1a1a;",
}


def style(tag):
    return f' style="{CSS[tag]}"'


def esc(text):
    return text.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def process_inline(text):
    text = re.sub(r'`([^`]+?)`',
                  lambda m: f'<code{style("icode")}>{esc(m.group(1))}</code>', text)
    text = re.sub(r'\*\*(.+?)\*\*',
                  lambda m: f'<strong{style("strong")}>{m.group(1)}</strong>', text)
    text = re.sub(r'\[([^\]]+)\]\(([^)]+)\)',
                  lambda m: f'<a href="{m.group(2)}" style="color:#07c160;text-decoration:none;">{m.group(1)}</a>', text)
    return text


def classify_bq(text):
    if any(kw in text for kw in ["⚠", "⚠️", "警告", "注意", "限制", "风险"]):
        return "bq_warn"
    if any(kw in text for kw in ["📡", "📬", "Dji-cloud-api-tool"]):
        return "bq_info"
    return "bq_tip"


def build_code_block(code_text):
    """Build a code block as line-by-line <p> elements in a scrollable <section>.
    Each line gets white-space:nowrap so it NEVER wraps, and the section scrolls horizontally."""
    lines = code_text.split("\n")
    line_els = []
    for line in lines:
        # Preserve leading/trailing spaces with &nbsp;
        # Count leading spaces
        stripped = line.lstrip(" ")
        lead = len(line) - len(stripped)
        if lead > 0:
            rendered = "&nbsp;" * lead + esc(stripped) if stripped else "&nbsp;" * lead
        else:
            rendered = esc(line) if line else "&nbsp;"  # empty line = single nbsp for height
        line_els.append(f'<p{style("code_line")}>{rendered}</p>')
    return f'<section{style("code_wrap")}>\n' + "\n".join(line_els) + "\n</section>"


def convert(md_text):
    lines = md_text.split("\n")
    out = []
    i = 0
    pending_li = []

    def flush_li():
        nonlocal pending_li
        if pending_li:
            out.append(f"<ul{style('ul')}>")
            for item in pending_li:
                out.append(item)
            out.append("</ul>")
            pending_li = []

    def is_sep(ln):
        return bool(re.match(r'^\|[\s\-:|]+\|$', ln.strip()))

    in_code = False
    code_buf = []

    in_table = False
    table_buf = []

    while i < len(lines):
        line = lines[i]

        # --- Code block ---
        if line.strip().startswith("```"):
            if in_code:
                flush_li()
                code = "\n".join(code_buf)
                out.append(build_code_block(code))
                code_buf = []
                in_code = False
            else:
                flush_li()
                in_code = True
            i += 1
            continue

        if in_code:
            code_buf.append(line)
            i += 1
            continue

        # --- Raw HTML (image table from intro) ---
        if line.strip().startswith("<table") or line.strip().startswith("<tr") or \
           line.strip().startswith("<td") or line.strip().startswith("</table") or \
           line.strip().startswith("</tr") or line.strip().startswith("</td"):
            flush_li()
            out.append(line)
            i += 1
            continue

        # --- Table ---
        if line.strip().startswith("|") and line.strip().endswith("|"):
            if not in_table:
                flush_li()
                in_table = True
            if is_sep(line):
                i += 1
                continue
            table_buf.append(line)
            i += 1
            continue
        else:
            if in_table:
                rows_html = []
                for ri, row in enumerate(table_buf):
                    cells = [c.strip() for c in row.split("|")[1:-1]]
                    tag = "th" if ri == 0 else "td"
                    tr_bg = "" if ri == 0 else ('background-color:#f9fdfa;' if ri % 2 == 0 else "")
                    rows_html.append(f'<tr style="{tr_bg}">')
                    for cell in cells:
                        cell_html = process_inline(cell)
                        rows_html.append(f"<{tag}{style(tag)}>{cell_html}</{tag}>")
                    rows_html.append("</tr>")
                out.append(f"<table{style('table')}>" + "".join(rows_html) + "</table>")
                table_buf = []
                in_table = False

        # --- Empty line ---
        if not line.strip():
            flush_li()
            out.append("")
            i += 1
            continue

        # --- HR ---
        if line.strip() == "---":
            flush_li()
            out.append('<p style="text-align:center;color:#ccc;font-size:12px;letter-spacing:8px;margin:20px 0;">&#8226; &#8226; &#8226;</p>')
            i += 1
            continue

        # --- Headings ---
        if line.startswith("#### "):
            flush_li()
            out.append(f"<h4{style('h4')}>{process_inline(line[5:].strip())}</h4>")
        elif line.startswith("### "):
            flush_li()
            out.append(f"<h3{style('h3')}>{process_inline(line[4:].strip())}</h3>")
        elif line.startswith("## "):
            flush_li()
            out.append(f"<h2{style('h2')}>{process_inline(line[3:].strip())}</h2>")
        elif line.startswith("# "):
            flush_li()
            out.append(f"<h1{style('h1')}>{process_inline(line[2:].strip())}</h1>")

        # --- Blockquote ---
        elif line.startswith("> "):
            flush_li()
            text = process_inline(line[2:].strip())
            cls = classify_bq(text)
            out.append(f"<blockquote{style(cls)}>{text}</blockquote>")

        # --- List items ---
        elif re.match(r'^[\-\*] ', line):
            text = process_inline(line[2:].strip())
            pending_li.append(f"<li{style('li')}>{text}</li>")

        # --- Regular paragraph ---
        else:
            flush_li()
            text = process_inline(line.strip())
            if text:
                out.append(f"<p{style('p')}>{text}</p>")

        i += 1

    flush_li()
    if in_code:
        out.append(build_code_block("\n".join(code_buf)))
    if in_table:
        rows_html = []
        for ri, row in enumerate(table_buf):
            cells = [c.strip() for c in row.split("|")[1:-1]]
            tag = "th" if ri == 0 else "td"
            tr_bg = "" if ri == 0 else ('background-color:#f9fdfa;' if ri % 2 == 0 else "")
            rows_html.append(f'<tr style="{tr_bg}">')
            for cell in cells:
                cell_html = process_inline(cell)
                rows_html.append(f"<{tag}{style(tag)}>{cell_html}</{tag}>")
            rows_html.append("</tr>")
        out.append(f"<table{style('table')}>" + "".join(rows_html) + "</table>")

    return "\n".join(out)


def build_html(body_html):
    return f"""<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<title>逐帧剖析 GB28181 推流：信令协商与媒体传输全解</title>
</head>
<body{style('body')}>
{body_html}
</body>
</html>"""


def main():
    with open(SRC, "r", encoding="utf-8") as f:
        md = f.read()
    body = convert(md)
    html = build_html(body)
    with open(DST, "w", encoding="utf-8") as f:
        f.write(html)
    print(f"Generated: {DST}")
    print(f"  Body:  {len(body)} chars")
    print(f"  Total: {len(html)} chars")


if __name__ == "__main__":
    main()
