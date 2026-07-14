#!/usr/bin/env python3
"""整理 user-guide.md 格式，使其符合微信公众号常用风格"""

import re
import sys
import io

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')

PATH = 'D:/project/damon/C/Dji-cloud-api-tool/Dji-cloud-api-tool/docs/user-guide.md'
text = open(PATH, encoding='utf-8').read()

# ══════════════════════════════════════════════════════════════
# 1. 去掉用作列表容器的代码块（内容以 1、 或 2、 开头）
# ══════════════════════════════════════════════════════════════

def unwrap_list_codeblock(m):
    content = m.group(1).strip()
    # 判断是否为列表型内容
    if re.match(r'^[1-9]、', content):
        return content
    if re.match(r'^- ', content):
        return content
    return m.group(0)

text = re.sub(r'```\n(.*?)\n```', unwrap_list_codeblock, text, flags=re.DOTALL)

# ══════════════════════════════════════════════════════════════
# 2. 移除过度使用的 > 引用标记
#    保留: > 💡、> ⚠️、> 👉、> A：
#    移除: 其他所有 > 前缀
# ══════════════════════════════════════════════════════════════

lines = text.split('\n')
result = []
i = 0

KEEP_PATTERNS = [r'^> (💡|⚠️|👉|A：)']

def should_keep(bq_lines):
    for line in bq_lines:
        for pat in KEEP_PATTERNS:
            if re.match(pat, line.strip()):
                return True
    return False

while i < len(lines):
    line = lines[i]

    if re.match(r'^> ', line):
        bq_lines = []
        while i < len(lines) and re.match(r'^> ', lines[i]):
            bq_lines.append(lines[i])
            i += 1

        if should_keep(bq_lines):
            for l in bq_lines:
                result.append(l)
        else:
            for l in bq_lines:
                stripped = re.sub(r'^> ', '', l)
                if stripped == '>':
                    stripped = ''
                result.append(stripped)
            if i < len(lines) and lines[i].strip():
                result.append('')
        continue

    result.append(line)
    i += 1

text = '\n'.join(result)

# ══════════════════════════════════════════════════════════════
# 3. 清理多余空行（最多连续 2 个空行）
# ══════════════════════════════════════════════════════════════

text = re.sub(r'\n{3,}', '\n\n', text)

# ══════════════════════════════════════════════════════════════
# 4. 确保表格前后各有一个空行（用 while 循环，避免 for+enumerate 中 i 不生效）
# ══════════════════════════════════════════════════════════════

lines = text.split('\n')
result = []
i = 0
while i < len(lines):
    stripped = lines[i].strip()
    is_table = stripped.startswith('|') and stripped.endswith('|')

    if is_table:
        # 确保表格前有空行
        if result and result[-1].strip():
            result.append('')
        # 输出整个表格
        while i < len(lines) and lines[i].strip().startswith('|') and lines[i].strip().endswith('|'):
            result.append(lines[i])
            i += 1
        # 确保表格后有空行
        if i < len(lines) and lines[i].strip():
            result.append('')
    else:
        result.append(lines[i])
        i += 1

text = '\n'.join(result)

# ══════════════════════════════════════════════════════════════
# 5. 末尾收尾 — 确保文件以单个换行结束
# ══════════════════════════════════════════════════════════════

text = text.rstrip('\n') + '\n'

open(PATH, 'w', encoding='utf-8').write(text)
print('Done! Format cleanup completed.')
