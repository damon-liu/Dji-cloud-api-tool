#!/usr/bin/env python3
"""
Generate WeChat-compatible HTML preview for GB28181 article.
Handles Mermaid diagrams (CDN for preview), SIP/SDP/XML code blocks,
ASCII art diagrams, and Typora local images (base64 embedded).

Usage: python gen_gb28181_wechat.py [--style wechat|minimal|deepblue]
"""

import re, os, sys, base64, json, zlib, urllib.request, ssl

STYLE = sys.argv[2] if len(sys.argv) > 2 and sys.argv[1] == "--style" else "wechat"

SRC = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                   "docs", "articles", "GB28181国标推流实战分析.md")
SUFFIX = {"wechat": "-wechat", "minimal": "-minimal", "deepblue": "-deepblue"}
DST = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                   "docs", "articles",
                   f"GB28181国标推流实战分析{SUFFIX.get(STYLE, '-wechat')}.html")

# ═══════════════════════════════════════════════════════════════════════
# Style definitions (all inline for WeChat compatibility)
# ═══════════════════════════════════════════════════════════════════════

if STYLE == "minimal":
    CSS = {
        "body": "margin:0;padding:16px 14px 36px;"
                "font-family:-apple-system,BlinkMacSystemFont,'PingFang SC','Hiragino Sans GB',"
                "'Microsoft YaHei','Noto Sans SC','Helvetica Neue',Arial,sans-serif;"
                "font-size:15px;color:#1a1a1a;line-height:1.9;letter-spacing:0.3px;"
                "background-color:#ffffff;",

        "h1": "text-align:center;font-size:23px;font-weight:700;color:#000;"
              "margin:22px 0 6px;padding-bottom:0;line-height:1.45;letter-spacing:0.8px;",
        "h1_sub": "text-align:center;font-size:13px;color:#999;margin:6px 0 28px;"
                  "font-weight:400;letter-spacing:0.5px;",

        "h2": "font-size:18px;font-weight:700;color:#000;margin:32px 0 12px;"
              "padding:0;line-height:1.45;border:none;background:none;",

        "h3": "font-size:16px;font-weight:600;color:#222;margin:22px 0 10px;"
              "padding:0;border:none;line-height:1.5;",

        "h4": "font-size:15px;font-weight:600;color:#444;margin:16px 0 8px;",

        "p": "margin:10px 0;text-align:justify;word-break:break-all;color:#333;",

        "table": "width:100%;border-collapse:collapse;margin:14px 0;font-size:13px;"
                 "border:1px solid #e8e8e8;border-radius:4px;overflow:hidden;",

        "th": "background-color:#f5f5f5;color:#222;padding:10px 12px;text-align:left;"
               "font-weight:600;font-size:13px;border-bottom:2px solid #ddd;",

        "td": "padding:9px 12px;border-bottom:1px solid #f0f0f0;font-size:13px;"
              "background-color:#fff;line-height:1.7;word-break:break-all;color:#444;",

        "code_wrap": "background-color:#f8f8f8;border-radius:4px;padding:14px 0;"
                     "margin:16px 0;overflow-x:scroll;-webkit-overflow-scrolling:touch;"
                     "border:1px solid #eaeaea;",

        "code_line": "margin:0;padding:2px 16px;font-size:13px;line-height:1.75;"
                     "font-family:'SFMono-Regular',Consolas,'Liberation Mono',Menlo,monospace;"
                     "color:#555;white-space:nowrap;display:block;text-align:left;",

        "icode": "font-family:'SFMono-Regular',Consolas,'Liberation Mono',Menlo,monospace;"
                 "background-color:#f2f2f2;padding:1px 6px;border-radius:3px;font-size:13px;"
                 "color:#c7254e;",

        "bq_warn": "border-left:3px solid #e74c3c;background-color:#fefaf9;padding:12px 16px;"
                   "margin:14px 0;color:#666;font-size:14px;border-radius:0 4px 4px 0;"
                   "line-height:1.75;",

        "bq_info": "border-left:3px solid #888;background-color:#fafafa;padding:12px 16px;"
                   "margin:14px 0;color:#666;font-size:14px;border-radius:0 4px 4px 0;"
                   "line-height:1.75;",

        "bq_tip": "border-left:3px solid #333;background-color:#fafafa;padding:12px 16px;"
                  "margin:14px 0;color:#555;font-size:14px;border-radius:0 4px 4px 0;"
                  "line-height:1.75;",

        "img": "max-width:100%;display:block;margin:16px auto 6px;",

        "fig_caption": "text-align:center;font-size:12px;color:#aaa;margin:4px 0 16px;",

        "ul": "margin:8px 0;padding-left:24px;",
        "ol": "margin:8px 0;padding-left:24px;",
        "li": "margin:4px 0;font-size:14px;line-height:1.75;color:#444;",

        "strong": "color:#000;",

        "mermaid_wrap": "background:#fafbfc;border:1px solid #e8e8e8;border-radius:6px;"
                        "padding:20px 16px 12px;margin:18px 0;text-align:center;overflow-x:auto;",

        "mermaid_note": "font-size:12px;color:#bbb;margin-top:10px;text-align:center;"
                        "font-style:italic;",

        "ascii_wrap": "background-color:#f5f5f5;border-radius:4px;padding:14px 16px;"
                      "margin:16px 0;overflow-x:scroll;-webkit-overflow-scrolling:touch;"
                      "border:1px solid #e8e8e8;",

        "ascii_line": "margin:0;padding:1px 0;font-size:12px;line-height:1.55;"
                      "font-family:'SFMono-Regular',Consolas,'Liberation Mono',Menlo,monospace;"
                      "color:#666;white-space:pre;display:block;text-align:left;",

        "hr_dots": "text-align:center;color:#ddd;font-size:11px;letter-spacing:10px;"
                   "margin:28px 0 20px;",

        "flow_box": "background-color:#fafafa;border:1px solid #e8e8e8;border-radius:4px;"
                    "padding:14px 18px;margin:16px 0;",
        "flow_line": "margin:3px 0;font-size:13px;font-family:'SFMono-Regular',Consolas,"
                     "'Liberation Mono',Menlo,monospace;color:#666;line-height:1.65;"
                     "white-space:nowrap;display:block;overflow-x:auto;",
    }
elif STYLE == "deepblue":
    CSS = {
        # ── Base ──────────────────────────────────────────────────────
        "body": "margin:0;padding:14px 12px 32px;"
                "font-family:-apple-system,BlinkMacSystemFont,'PingFang SC','Hiragino Sans GB',"
                "'Microsoft YaHei','Noto Sans SC','Helvetica Neue',Arial,sans-serif;"
                "font-size:15px;color:#2a2d34;line-height:1.85;letter-spacing:0.4px;"
                "background-color:#ffffff;",

        # ── Headings ──────────────────────────────────────────────────
        "h1": "text-align:center;font-size:23px;font-weight:800;color:#0d1b2a;"
              "margin:24px 0 18px;padding-bottom:0;line-height:1.4;letter-spacing:1px;"
              "position:relative;",
        "h1_hr": "width:48px;height:3px;background:linear-gradient(90deg,#1b4965,#4895c8);"
                 "border:none;margin:6px auto 0;border-radius:2px;",

        "h2": "font-size:18px;font-weight:700;color:#1b2d45;margin:32px 0 14px;"
              "padding:10px 0 10px 16px;border-left:4px solid #1b4965;"
              "background:linear-gradient(90deg,#f0f4f8,rgba(240,244,248,0));"
              "line-height:1.5;border-radius:0 4px 4px 0;",

        "h3": "font-size:16px;font-weight:700;color:#2c3e50;margin:22px 0 10px;"
              "padding-left:10px;border-left:3px solid #4895c8;line-height:1.5;",

        "h4": "font-size:15px;font-weight:700;color:#3d5068;margin:16px 0 8px;",

        # ── Paragraph ─────────────────────────────────────────────────
        "p": "margin:8px 0;text-align:justify;word-break:break-all;color:#3a3f47;",

        # ── Table ─────────────────────────────────────────────────────
        "table": "width:100%;border-collapse:collapse;margin:14px 0;font-size:13px;"
                 "box-shadow:0 2px 12px rgba(27,73,101,.06);border-radius:6px;overflow:hidden;",

        "th": "background:linear-gradient(180deg,#1b4965,#1d3a55);color:#e8f0f8;"
               "padding:10px 12px;text-align:left;font-weight:600;font-size:13px;"
               "white-space:nowrap;",

        "td": "padding:9px 12px;border-bottom:1px solid #eef2f6;font-size:13px;"
              "background-color:#fff;line-height:1.7;word-break:break-all;color:#3a3f47;",

        # ── Code block ────────────────────────────────────────────────
        "code_wrap": "background-color:#0d1b2a;border-radius:8px;padding:14px 0;"
                     "margin:16px 0;overflow-x:scroll;-webkit-overflow-scrolling:touch;"
                     "border:1px solid #c8d6e5;",

        "code_line": "margin:0;padding:2px 18px;font-size:13px;line-height:1.75;"
                     "font-family:'SFMono-Regular',Consolas,'Liberation Mono',Menlo,monospace;"
                     "color:#c8dce8;white-space:nowrap;display:block;text-align:left;",

        "icode": "font-family:'SFMono-Regular',Consolas,'Liberation Mono',Menlo,monospace;"
                 "background-color:#eef2f6;padding:2px 7px;border-radius:4px;font-size:13px;"
                 "color:#c0392b;border:1px solid #d5dde5;",

        # ── Blockquote variants ───────────────────────────────────────
        "bq_warn": "border-left:4px solid #c0392b;background-color:#fef8f7;"
                   "padding:12px 16px;margin:14px 0;color:#5a4035;font-size:14px;"
                   "border-radius:0 6px 6px 0;line-height:1.75;",

        "bq_info": "border-left:4px solid #4895c8;background-color:#f4f8fc;"
                   "padding:12px 16px;margin:14px 0;color:#4a5568;font-size:14px;"
                   "border-radius:0 6px 6px 0;line-height:1.75;",

        "bq_tip": "border-left:4px solid #1b4965;background-color:#f0f5fa;"
                  "padding:12px 16px;margin:14px 0;color:#3d5068;font-size:14px;"
                  "border-radius:0 6px 6px 0;line-height:1.75;",

        # ── Image ─────────────────────────────────────────────────────
        "img": "max-width:100%;display:block;margin:16px auto 6px;border-radius:6px;"
               "box-shadow:0 3px 12px rgba(0,0,0,.07);",

        "fig_caption": "text-align:center;font-size:12px;color:#8899aa;margin:4px 0 16px;",

        # ── Lists ─────────────────────────────────────────────────────
        "ul": "margin:8px 0;padding-left:24px;",
        "ol": "margin:8px 0;padding-left:24px;",
        "li": "margin:4px 0;font-size:14px;line-height:1.75;color:#3a3f47;",

        # ── Bold ──────────────────────────────────────────────────────
        "strong": "color:#0d1b2a;",

        # ── Mermaid ───────────────────────────────────────────────────
        "mermaid_wrap": "background:#f6f9fc;border:1px solid #d5dde5;border-radius:10px;"
                        "padding:20px 16px 12px;margin:18px 0;text-align:center;overflow-x:auto;",

        "mermaid_note": "font-size:12px;color:#a0b0c0;margin-top:10px;text-align:center;"
                        "font-style:italic;",

        # ── ASCII art ─────────────────────────────────────────────────
        "ascii_wrap": "background-color:#0d1b2a;border-radius:8px;padding:14px 18px;"
                      "margin:16px 0;overflow-x:scroll;-webkit-overflow-scrolling:touch;"
                      "border:1px solid #c8d6e5;",

        "ascii_line": "margin:0;padding:1px 0;font-size:12px;line-height:1.55;"
                      "font-family:'SFMono-Regular',Consolas,'Liberation Mono',Menlo,monospace;"
                      "color:#c8dce8;white-space:pre;display:block;text-align:left;",

        # ── HR ────────────────────────────────────────────────────────
        "hr_dots": "text-align:center;color:#c8d6e5;font-size:11px;letter-spacing:10px;"
                   "margin:28px 0 20px;",

        # ── Flow box ──────────────────────────────────────────────────
        "flow_box": "background-color:#f6f9fc;border:1px solid #d5dde5;border-radius:8px;"
                    "padding:14px 18px;margin:16px 0;",
        "flow_line": "margin:3px 0;font-size:13px;font-family:'SFMono-Regular',Consolas,"
                     "'Liberation Mono',Menlo,monospace;color:#4a5568;line-height:1.65;"
                     "white-space:nowrap;display:block;overflow-x:auto;",
    }
else:
    CSS = {
    # ── Base ──────────────────────────────────────────────────────────
    "body": "margin:0;padding:14px 12px 32px;"
            "font-family:-apple-system,BlinkMacSystemFont,'PingFang SC','Hiragino Sans GB',"
            "'Microsoft YaHei','Noto Sans SC','Helvetica Neue',Arial,sans-serif;"
            "font-size:15px;color:#2c2c2c;line-height:1.85;letter-spacing:0.4px;"
            "background-color:#ffffff;",

    # ── Headings ──────────────────────────────────────────────────────
    "h1": "text-align:center;font-size:22px;font-weight:800;color:#0f0f0f;"
          "margin:20px 4px 14px;padding-bottom:16px;border-bottom:2px solid #07c160;"
          "line-height:1.45;letter-spacing:1.2px;",

    "h2": "font-size:18px;font-weight:700;color:#1a1a1a;margin:30px 0 14px;"
          "padding:8px 0 8px 14px;border-left:4px solid #07c160;"
          "background-color:#f0faf4;line-height:1.5;border-radius:0 4px 4px 0;",

    "h3": "font-size:16px;font-weight:700;color:#2a2a2a;margin:22px 0 10px;"
          "padding-left:10px;border-left:3px solid #52c97d;line-height:1.5;",

    "h4": "font-size:15px;font-weight:700;color:#444;margin:16px 0 8px;",

    # ── Paragraph / Text ──────────────────────────────────────────────
    "p": "margin:8px 0;text-align:justify;word-break:break-all;",

    # ── Table ─────────────────────────────────────────────────────────
    "table": "width:100%;border-collapse:collapse;margin:14px 0;font-size:13px;"
             "box-shadow:0 2px 8px rgba(0,0,0,.06);border-radius:6px;overflow:hidden;",

    "th": "background-color:#07c160;color:#fff;padding:10px 12px;text-align:left;"
           "font-weight:600;font-size:13px;white-space:nowrap;",

    "td": "padding:9px 12px;border-bottom:1px solid #f0f0f0;font-size:13px;"
          "background-color:#fff;line-height:1.7;word-break:break-all;",

    # ── Code block (line-by-line <p>, scrollable) ─────────────────────
    "code_wrap": "background-color:#1e2230;border-radius:8px;padding:14px 0;"
                 "margin:16px 0;overflow-x:scroll;-webkit-overflow-scrolling:touch;"
                 "border:1px solid #e4e4e4;",

    "code_line": "margin:0;padding:2px 18px;font-size:13px;line-height:1.75;"
                 "font-family:'SFMono-Regular',Consolas,'Liberation Mono',Menlo,monospace;"
                 "color:#c8d6e5;white-space:nowrap;display:block;text-align:left;",

    "icode": "font-family:'SFMono-Regular',Consolas,'Liberation Mono',Menlo,monospace;"
             "background-color:#f4f4f4;padding:2px 7px;border-radius:4px;font-size:13px;"
             "color:#c7254e;border:1px solid #e8e8e8;",

    # ── Blockquote variants ───────────────────────────────────────────
    "bq_warn": "border-left:4px solid #e74c3c;background-color:#fef6f5;padding:12px 16px;"
               "margin:14px 0;color:#555;font-size:14px;border-radius:0 6px 6px 0;"
               "line-height:1.75;",

    "bq_info": "border-left:4px solid #5b9bd5;background-color:#f4f8fd;padding:12px 16px;"
               "margin:14px 0;color:#555;font-size:14px;border-radius:0 6px 6px 0;"
               "line-height:1.75;",

    "bq_tip": "border-left:4px solid #07c160;background-color:#f0faf4;padding:12px 16px;"
              "margin:14px 0;color:#444;font-size:14px;border-radius:0 6px 6px 0;"
              "line-height:1.75;",

    # ── Image ─────────────────────────────────────────────────────────
    "img": "max-width:100%;display:block;margin:16px auto 6px;border-radius:6px;"
           "box-shadow:0 2px 8px rgba(0,0,0,.08);",

    "fig_caption": "text-align:center;font-size:12px;color:#999;margin:4px 0 16px;",

    # ── Lists ─────────────────────────────────────────────────────────
    "ul": "margin:8px 0;padding-left:24px;",
    "ol": "margin:8px 0;padding-left:24px;",
    "li": "margin:4px 0;font-size:14px;line-height:1.75;",

    # ── Bold ──────────────────────────────────────────────────────────
    "strong": "color:#111;",

    # ── Mermaid diagram container ─────────────────────────────────────
    "mermaid_wrap": "background:#f8f9fb;border:1px solid #e4e7ed;border-radius:10px;"
                    "padding:20px 16px 12px;margin:18px 0;text-align:center;overflow-x:auto;",

    "mermaid_note": "font-size:12px;color:#b0b0b0;margin-top:10px;text-align:center;"
                    "font-style:italic;",

    # ── ASCII art diagram (dark theme) ────────────────────────────────
    "ascii_wrap": "background-color:#1e2230;border-radius:8px;padding:14px 18px;"
                  "margin:16px 0;overflow-x:scroll;-webkit-overflow-scrolling:touch;"
                  "border:1px solid #d0d0d0;",

    "ascii_line": "margin:0;padding:1px 0;font-size:12px;line-height:1.55;"
                  "font-family:'SFMono-Regular',Consolas,'Liberation Mono',Menlo,monospace;"
                  "color:#c8d6e5;white-space:pre;display:block;text-align:left;",

    # ── Horizontal rule ───────────────────────────────────────────────
    "hr_dots": "text-align:center;color:#d0d0d0;font-size:11px;letter-spacing:10px;"
               "margin:28px 0 20px;",

    # ── Flow / timeline box ───────────────────────────────────────────
    "flow_box": "background-color:#f8faf8;border:1px solid #d8e8d8;border-radius:8px;"
                "padding:14px 18px;margin:16px 0;",
    "flow_line": "margin:3px 0;font-size:13px;font-family:'SFMono-Regular',Consolas,"
                 "'Liberation Mono',Menlo,monospace;color:#555;line-height:1.65;"
                 "white-space:nowrap;display:block;overflow-x:auto;",
}


def style(tag):
    return f' style="{CSS[tag]}"'


def esc(text):
    return text.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def process_inline(text):
    """Process inline markdown: `code`, **bold**, [links](url)"""
    text = re.sub(r'`([^`]+?)`',
                  lambda m: f'<code{style("icode")}>{esc(m.group(1))}</code>', text)
    text = re.sub(r'\*\*(.+?)\*\*',
                  lambda m: f'<strong{style("strong")}>{m.group(1)}</strong>', text)
    text = re.sub(r'\[([^\]]+)\]\(([^)]+)\)',
                  lambda m: f'<a href="{m.group(2)}" style="color:#07c160;text-decoration:none;">{m.group(1)}</a>', text)
    return text


def classify_bq(text):
    """Color-code blockquotes by content."""
    if any(kw in text for kw in ["⚠", "⚠️", "警告", "注意", "NAT", "限制", "风险", "Error"]):
        return "bq_warn"
    if any(kw in text for kw in ["说明", "Note", "建议", "提示"]):
        return "bq_info"
    if any(kw in text for kw in ["实战要点", "关键", "核心"]):
        return "bq_tip"
    return "bq_info"


def build_code_block(code_text, lang=""):
    """Build a code block as line-by-line <p> elements in a scrollable <section>.
    Adds a subtle header bar with language label when available."""
    lines = code_text.split("\n")
    if lines and lines[-1] == "":
        lines = lines[:-1]

    parts = []

    # Language header bar (only when lang is specified)
    if lang:
        # Style-specific header colors
        header_colors = {
            "wechat":   ("#252a38", "#7ec98a"),
            "minimal":  ("#eeeeee", "#888888"),
            "deepblue": ("#12253a", "#5bafd8"),
        }
        hdr_bg, hdr_fg = header_colors.get(STYLE, header_colors["wechat"])
        label = lang.upper()
        parts.append(f'<div style="background-color:{hdr_bg};border-radius:8px 8px 0 0;'
                     f'padding:6px 18px;margin:16px 0 0 0;font-size:11px;'
                     f'font-family:\'SFMono-Regular\',Consolas,Menlo,monospace;'
                     f'color:{hdr_fg};letter-spacing:1.5px;font-weight:600;">'
                     f'{esc(label)}</div>')

    # Code lines
    line_els = []
    for line in lines:
        stripped = line.lstrip(" ")
        lead = len(line) - len(stripped)
        if lead > 0:
            rendered = "&nbsp;" * lead + esc(stripped) if stripped else "&nbsp;" * lead
        else:
            rendered = esc(line) if line else "&nbsp;"
        line_els.append(f'<p{style("code_line")}>{rendered}</p>')

    if lang:
        wrap_colors = {
            "wechat":   ("#1e2230", "#e4e4e4"),
            "minimal":  ("#f8f8f8", "#eaeaea"),
            "deepblue": ("#0d1b2a", "#c8d6e5"),
        }
        wrap_bg, wrap_border = wrap_colors.get(STYLE, wrap_colors["wechat"])
        wrap_style = (f"background-color:{wrap_bg};border-radius:0 0 8px 8px;padding:10px 0;"
                      f"margin:0 0 16px 0;overflow-x:scroll;-webkit-overflow-scrolling:touch;"
                      f"border:1px solid {wrap_border};border-top:none;")
        parts.append(f'<section style="{wrap_style}">\n' + "\n".join(line_els) + "\n</section>")
    else:
        parts.append(f'<section{style("code_wrap")}>\n' + "\n".join(line_els) + "\n</section>")

    return "\n".join(parts)


def build_ascii_diagram(text):
    """Build an ASCII art diagram in dark-themed code block."""
    lines = text.strip().split("\n")
    line_els = []
    for line in lines:
        rendered = esc(line) if line else "&nbsp;"
        line_els.append(f'<p{style("ascii_line")}>{rendered}</p>')
    return f'<section{style("ascii_wrap")}>\n' + "\n".join(line_els) + "\n</section>"


def build_table(rows):
    """Build a WeChat-styled table from markdown rows.
    rows: list of strings like "| a | b | c |"
    """
    rows_html = []
    for ri, row in enumerate(rows):
        cells = [c.strip() for c in row.split("|")[1:-1]]
        tag = "th" if ri == 0 else "td"
        tr_bg = "" if ri == 0 else ('background-color:#f9fdfa;' if ri % 2 == 0 else "")
        rows_html.append(f'<tr style="{tr_bg}">')
        for cell in cells:
            cell_html = process_inline(cell)
            rows_html.append(f"<{tag}{style(tag)}>{cell_html}</{tag}>")
        rows_html.append("</tr>")
    return f"<table{style('table')}>\n" + "\n".join(rows_html) + "\n</table>"


def download_mermaid_png(diagram_text, out_path):
    """Download Mermaid diagram as PNG from mermaid.ink, save to out_path.
    Removes <br/> tags from diagram text (mermaid.ink doesn't handle them).
    Returns True on success."""
    # Clean <br/> from participant names — mermaid.ink can't parse them
    cleaned = re.sub(r'<br\s*/?>', ' ', diagram_text.strip())

    def try_download(url):
        ctx = ssl.create_default_context()
        req = urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0"})
        with urllib.request.urlopen(req, timeout=30, context=ctx) as resp:
            if resp.status == 200:
                with open(out_path, "wb") as f:
                    f.write(resp.read())
                return True
        return False

    # Method 1: pako-style compressed encoding
    try:
        payload = json.dumps({"code": cleaned}, separators=(",", ":"))
        raw = zlib.compress(payload.encode("utf-8"), level=9)[2:-4]
        encoded = base64.urlsafe_b64encode(raw).decode("ascii").rstrip("=")
        if try_download(f"https://mermaid.ink/img/pako:{encoded}?type=png"):
            return True
    except Exception:
        pass

    # Method 2: plain base64
    try:
        encoded = base64.urlsafe_b64encode(cleaned.encode("utf-8")).decode("ascii").rstrip("=")
        if try_download(f"https://mermaid.ink/img/{encoded}?type=png"):
            return True
    except Exception:
        pass

    return False


def build_mermaid_placeholder(diagram_text, index):
    """Build a Mermaid diagram as a local PNG file, referenced by absolute path.
    The push script will upload it to WeChat CDN along with other images."""
    articles_dir = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                                "docs", "articles")
    png_name = f"mermaid_diagram_{index + 1}.png"
    png_path = os.path.join(articles_dir, png_name)

    print(f"      Rendering Mermaid diagram {index + 1} via mermaid.ink...")
    if download_mermaid_png(diagram_text, png_path):
        size_kb = os.path.getsize(png_path) / 1024
        print(f"      OK: diagram {index + 1} saved ({size_kb:.0f} KB) -> {png_name}")
        # Use absolute Windows path so push script can match and replace
        return f"""<div{style("mermaid_wrap")}>
<img{style("img")} src="{png_path.replace(chr(92), chr(92))}" alt="信令流程图">
<p{style("mermaid_note")}>▲ 国标推流信令交互流程图</p>
</div>"""

    # Fallback: mermaid.ink URL
    print(f"      WARN: diagram {index + 1} download failed, using URL fallback")
    try:
        payload = json.dumps({"code": re.sub(r'<br\s*/?>', ' ', diagram_text.strip())},
                             separators=(",", ":"))
        raw = zlib.compress(payload.encode("utf-8"), level=9)[2:-4]
        encoded = base64.urlsafe_b64encode(raw).decode("ascii").rstrip("=")
        url = f"https://mermaid.ink/img/pako:{encoded}?type=png"
    except Exception:
        url = ""
    return f"""<div{style("mermaid_wrap")}>
{'<img style="max-width:100%;display:block;margin:0 auto;" src="' + url + '" alt="流程图">' if url else ''}
<p{style("mermaid_note")}>▲ 流程图</p>
</div>"""


def try_resolve_image_path(img_path):
    """Try to resolve an image path to an absolute filesystem path.
    Returns the absolute path if found, or None."""
    project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    candidates = [
        img_path,  # absolute path
        os.path.join(project_root, img_path),  # relative to project
        os.path.join(os.path.dirname(SRC), os.path.basename(img_path)),  # same dir as md
    ]
    for p in candidates:
        p = os.path.normpath(p)
        if os.path.isfile(p):
            return p
    return None


def convert(md_text):
    lines = md_text.split("\n")
    out = []
    i = 0
    pending_li = []
    mermaid_idx = 0

    def flush_li():
        nonlocal pending_li
        if pending_li:
            out.append(f"<ul{style('ul')}>")
            for item in pending_li:
                out.append(item)
            out.append("</ul>")
            pending_li = []

    in_code = False
    code_buf = []
    code_lang = ""

    in_table = False
    table_buf = []

    in_mermaid = False
    mermaid_buf = []

    # Track for multi-line blockquotes (consecutive > lines)
    in_bq = False
    bq_buf = []
    bq_type = "bq_info"

    def flush_bq():
        nonlocal in_bq, bq_buf, bq_type
        if bq_buf:
            icons = {"bq_warn": "⚠️ ", "bq_tip": "💡 ", "bq_info": "ℹ️ "}
            icon = icons.get(bq_type, "")
            text = icon + "<br>".join(bq_buf)
            out.append(f"<blockquote{style(bq_type)}>{text}</blockquote>")
            bq_buf = []
            bq_type = "bq_info"
            in_bq = False

    while i < len(lines):
        line = lines[i]

        # --- Mermaid block ---
        if line.strip().startswith("```mermaid"):
            flush_li()
            flush_bq()
            in_mermaid = True
            mermaid_buf = []
            i += 1
            continue

        if in_mermaid:
            if line.strip().startswith("```"):
                diagram = "\n".join(mermaid_buf)
                out.append(build_mermaid_placeholder(diagram, mermaid_idx))
                mermaid_idx += 1
                mermaid_buf = []
                in_mermaid = False
                i += 1
                continue
            mermaid_buf.append(line)
            i += 1
            continue

        # --- Code block ---
        if line.strip().startswith("```"):
            if in_code:
                flush_li()
                flush_bq()
                code = "\n".join(code_buf)
                out.append(build_code_block(code, code_lang))
                code_buf = []
                code_lang = ""
                in_code = False
            else:
                flush_li()
                flush_bq()
                in_code = True
                code_lang = line.strip()[3:].strip()
            i += 1
            continue

        if in_code:
            code_buf.append(line)
            i += 1
            continue

        # --- Table ---
        if line.strip().startswith("|") and line.strip().endswith("|"):
            if not in_table:
                flush_li()
                flush_bq()
                in_table = True
            # Skip separator rows like |---|---|
            if re.match(r'^\|[\s\-:|]+\|$', line.strip()):
                i += 1
                continue
            table_buf.append(line)
            i += 1
            continue
        else:
            if in_table:
                out.append(build_table(table_buf))
                table_buf = []
                in_table = False

        # --- Empty line ---
        if not line.strip():
            if in_bq:
                flush_bq()
            flush_li()
            out.append("")
            i += 1
            continue

        # --- HR ---
        if line.strip() == "---":
            flush_li()
            flush_bq()
            out.append(f'<p{style("hr_dots")}>&#8226; &#8226; &#8226;</p>')
            i += 1
            continue

        # --- Headings ---
        h4m = re.match(r'^#### (.+)', line)
        h3m = re.match(r'^### (.+)', line)
        h2m = re.match(r'^## (.+)', line)
        h1m = re.match(r'^# (.+)', line)

        if h4m:
            flush_li()
            flush_bq()
            out.append(f"<h4{style('h4')}>{process_inline(h4m.group(1))}</h4>")
        elif h3m:
            flush_li()
            flush_bq()
            out.append(f"<h3{style('h3')}>{process_inline(h3m.group(1))}</h3>")
        elif h2m:
            flush_li()
            flush_bq()
            out.append(f"<h2{style('h2')}>{process_inline(h2m.group(1))}</h2>")
        elif h1m:
            flush_li()
            flush_bq()
            out.append(f"<h1{style('h1')}>{process_inline(h1m.group(1))}</h1>")

        # --- Blockquote ---
        elif line.startswith("> "):
            flush_li()
            text = process_inline(line[2:].strip())
            if not in_bq:
                in_bq = True
                bq_buf = [text]
                bq_type = classify_bq(text)
            else:
                # Check if next line is also a blockquote of same content type
                bq_buf.append(text)
                new_type = classify_bq(text)
                if new_type != "bq_info":  # only upgrade to warn/tip
                    bq_type = new_type

        # --- Image (standalone) ---
        elif re.match(r'^!\[([^\]]*)\]\(([^)]+)\)$', line.strip()):
            flush_li()
            flush_bq()
            m = re.match(r'^!\[([^\]]*)\]\(([^)]+)\)$', line.strip())
            alt_text = m.group(1)
            img_path = m.group(2)
            # Resolve to absolute path for push script to upload
            resolved = try_resolve_image_path(img_path)
            if resolved:
                out.append(f'<img{style("img")} src="{resolved}" alt="{esc(alt_text)}">')
                out.append(f'<p{style("fig_caption")}>▲ {esc(alt_text)}</p>')
            else:
                # Fallback: note the image path
                out.append(f'<div style="background:#fef9e7;border:1px solid #f0c040;border-radius:8px;padding:14px 18px;margin:16px 0;text-align:center;color:#7a6500;font-size:13px;">'
                           f'📷 <strong>图片占位：</strong>{esc(alt_text or os.path.basename(img_path))}<br>'
                           f'<span style="font-size:11px;opacity:.65;">源路径：{esc(img_path)}</span></div>')

        # --- List items ---
        elif re.match(r'^[\-\*] ', line):
            flush_bq()
            text = process_inline(line[2:].strip())
            pending_li.append(f"<li{style('li')}>{text}</li>")

        # --- Regular paragraph ---
        else:
            flush_li()
            flush_bq()
            text = process_inline(line.strip())
            if text:
                out.append(f"<p{style('p')}>{text}</p>")

        i += 1

    # Flush remaining buffers
    flush_li()
    flush_bq()
    if in_code:
        out.append(build_code_block("\n".join(code_buf), code_lang))
    if in_table:
        out.append(build_table(table_buf))
    if in_mermaid:
        diagram = "\n".join(mermaid_buf)
        out.append(build_mermaid_placeholder(diagram, mermaid_idx))

    return "\n".join(out)


def build_html(body_html, title="逐帧剖析 GB28181 推流"):
    return f"""<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<title>{title}</title>
<script src="https://cdn.jsdelivr.net/npm/mermaid@10/dist/mermaid.min.js"></script>
<script>
mermaid.initialize({{ startOnLoad: true, theme: 'default', securityLevel: 'loose' }});
</script>
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
    print(f"[OK] Generated: {DST}")
    print(f"   Body chars:  {len(body):,}")
    print(f"   Total chars: {len(html):,}")
    print()
    print("Next steps for WeChat publish:")
    print(f"   1. Open {DST} in browser to preview")
    print("   2. Convert Mermaid diagrams to PNG images (use mermaid.live or CLI)")
    print('   3. Replace <pre class="mermaid"> blocks with <img> tags')
    print("   4. Upload images to WeChat CDN via /cgi-bin/media/uploadimg")
    print("   5. Push draft via scripts/push_wechat_draft.py")


if __name__ == "__main__":
    main()
