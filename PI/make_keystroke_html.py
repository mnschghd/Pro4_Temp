#!/usr/bin/env python3
import re
import html as html_module

def parse_tokens(line):
    tokens = []
    for m in re.finditer(r'\[([^\]]+)\]|(.)', line):
        if m.group(1):
            tokens.append(('special', '[' + m.group(1) + ']'))
        else:
            tokens.append(('text', m.group(2)))
    return tokens

def classify_key(key):
    k = key.lower()
    if 'ctrl' in k:
        return 'ctrl'
    elif 'alt' in k:
        return 'alt'
    elif key in ('[Backspace]', '[Delete]', '[Del]'):
        return 'delete'
    elif key in ('[Enter]', '[Return]'):
        return 'enter'
    elif key in ('[CapsLock]', '[NumLock]', '[ScrollLock]'):
        return 'modifier'
    elif key in ('[Home]', '[End]', '[PageUp]', '[PageDown]',
                 '[Up]', '[Down]', '[Left]', '[Right]'):
        return 'navigation'
    elif key in ('[Tab]',):
        return 'tab'
    elif key in ('[Esc]', '[Escape]'):
        return 'escape'
    elif re.match(r'\[F\d+\]', key):
        return 'function'
    else:
        return 'other'

COLORS = {
    'ctrl':       ('#ff6b6b', '#000'),
    'alt':        ('#ffa07a', '#000'),
    'delete':     ('#ffd700', '#000'),
    'enter':      ('#6ddc8b', '#000'),
    'modifier':   ('#da70d6', '#000'),
    'navigation': ('#87ceeb', '#000'),
    'tab':        ('#98fb98', '#000'),
    'escape':     ('#ff4500', '#fff'),
    'function':   ('#b0c4de', '#000'),
    'other':      ('#c8c8c8', '#000'),
}

def tokens_to_html(tokens):
    parts = []
    for type_, val in tokens:
        if type_ == 'text':
            parts.append(f'<span class="text">{html_module.escape(val)}</span>')
        else:
            cat = classify_key(val)
            bg, fg = COLORS.get(cat, ('#c8c8c8', '#000'))
            label = html_module.escape(val)
            parts.append(
                f'<span class="key key-{cat}" '
                f'style="background:{bg};color:{fg}" '
                f'title="{label}">{label}</span>'
            )
    return ''.join(parts)

def reconstruct(tokens):
    """Best-effort reconstruction: apply Backspace, Enter, CapsLock."""
    buf = []
    caps = False
    for type_, val in tokens:
        if type_ == 'text':
            c = val.upper() if (caps and val.islower()) else (val.lower() if (caps and val.isupper()) else val)
            buf.append(c)
        else:
            if val == '[Backspace]':
                if buf:
                    buf.pop()
            elif val == '[Enter]':
                buf.append('\n')
            elif val == '[CapsLock]':
                caps = not caps
            elif val == '[Tab]':
                buf.append('\t')
    return ''.join(buf)

def count_special(tokens):
    counts = {}
    for type_, val in tokens:
        if type_ == 'special':
            counts[val] = counts.get(val, 0) + 1
    return sorted(counts.items(), key=lambda x: -x[1])

with open('/home/elias/Downloads/keystrokes.txt') as f:
    raw_lines = f.read().splitlines()

line_data = []
for i, line in enumerate(raw_lines, 1):
    tokens = parse_tokens(line)
    line_data.append({
        'num': i,
        'tokens': tokens,
        'html': tokens_to_html(tokens),
        'reconstructed': reconstruct(tokens),
        'special_counts': count_special(tokens),
    })

legend_items = [
    ('#ff6b6b', 'Ctrl combos'),
    ('#ffd700', 'Backspace / Delete'),
    ('#6ddc8b', 'Enter'),
    ('#da70d6', 'CapsLock / Modifiers'),
    ('#87ceeb', 'Navigation (Home/End/Arrows)'),
    ('#ffa07a', 'Alt combos'),
    ('#c8c8c8', 'Other special'),
]

page = '''<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title>Keystroke Log Viewer</title>
<style>
  * { box-sizing: border-box; }
  body {
    background: #1e1e1e; color: #d4d4d4;
    font-family: "Courier New", monospace; font-size: 14px;
    padding: 24px; line-height: 1.9; margin: 0;
  }
  h1 { color: #569cd6; margin-bottom: 4px; }
  h2 { color: #569cd6; margin-top: 28px; border-bottom: 1px solid #3c3c3c; padding-bottom: 6px; }
  .subtitle { color: #858585; font-size: 12px; margin-bottom: 20px; }
  .legend { display: flex; flex-wrap: wrap; gap: 10px; margin: 12px 0 24px; }
  .legend-item { display: flex; align-items: center; gap: 6px; font-size: 12px; }
  .swatch { width: 14px; height: 14px; border-radius: 3px; border: 1px solid rgba(255,255,255,.15); flex-shrink: 0; }
  .section { background: #252526; border: 1px solid #3c3c3c; border-radius: 6px; padding: 16px 20px; margin: 12px 0; }
  .line-block { margin-bottom: 20px; }
  .line-label { color: #858585; font-size: 11px; margin-bottom: 4px; }
  .keystroke-line { word-break: break-all; line-height: 2.2; }
  .key {
    border-radius: 4px; padding: 1px 5px; margin: 0 1px;
    font-size: 11px; font-weight: bold;
    border: 1px solid rgba(0,0,0,.25);
    display: inline-block; white-space: nowrap; vertical-align: middle;
  }
  .text { color: #d4d4d4; }
  .reconstructed {
    white-space: pre-wrap; color: #9cdcfe;
    background: #1e1e2e; border-radius: 4px;
    padding: 10px 14px; margin-top: 8px;
  }
  .stats { font-size: 12px; color: #858585; margin-top: 8px; }
  .stats table { border-collapse: collapse; margin-top: 6px; }
  .stats td { padding: 2px 12px 2px 0; }
  .stats td:first-child { color: #c8c8c8; }
  .note { background: #2d2d1e; border: 1px solid #666030; border-radius: 6px;
          padding: 10px 14px; margin-top: 16px; font-size: 12px; color: #d7ba7d; }
</style>
</head>
<body>
<h1>Keystroke Log Viewer</h1>
<p class="subtitle">Color-coded keystroke log with reconstruction</p>

<h2>Legend</h2>
<div class="legend">
'''

for color, label in legend_items:
    page += f'  <div class="legend-item"><div class="swatch" style="background:{color}"></div><span>{label}</span></div>\n'

page += '</div>\n<h2>Keystrokes by Line</h2>\n'

for ld in line_data:
    page += f'<div class="line-block">\n'
    page += f'  <div class="line-label">Line {ld["num"]}</div>\n'
    page += f'  <div class="section">\n'
    page += f'    <div class="keystroke-line">{ld["html"]}</div>\n'
    page += f'    <div class="stats">\n'
    page += f'      <strong>Notable keys pressed:</strong>\n'
    page += f'      <table>\n'
    for key, count in ld['special_counts']:
        cat = classify_key(key)
        bg, fg = COLORS.get(cat, ('#c8c8c8', '#000'))
        badge = (f'<span class="key" style="background:{bg};color:{fg}">'
                 f'{html_module.escape(key)}</span>')
        page += f'        <tr><td>{badge}</td><td>×{count}</td></tr>\n'
    page += f'      </table>\n'
    page += f'    </div>\n'
    page += f'    <div style="margin-top:10px;font-size:12px;color:#858585;">Reconstructed text <em>(Backspace/Enter/CapsLock applied; navigation treated as no-op)</em>:</div>\n'
    page += f'    <div class="reconstructed">{html_module.escape(ld["reconstructed"])}</div>\n'
    page += f'  </div>\n'
    page += f'</div>\n'

# interesting observations
page += '''
<h2>Observations</h2>
<div class="note">
  <strong>Line 1:</strong> 7× [Ctrl+c] in quick succession — likely mashing Ctrl+C to interrupt a running process.<br>
  [Ctrl+Ins] = copy shortcut (alternative to Ctrl+C in some apps).<br>
  [Home] then [Backspace] — moved cursor to start of line, then backspaced (no-op at position 0 in most shells).<br>
  Multiple [Backspace] clusters show text corrections mid-typing.
</div>
<div class="note">
  <strong>Line 2:</strong> Starts with uppercase characters (<code>sUDL ZDIF WGFLJ</code>) — CapsLock was active.<br>
  [CapsLock] mid-line turns it off, then lowercase resumes.<br>
  The content looks like it may have been typed in the wrong window (password/sensitive text accidentally typed into a logger).
</div>
</body>
</html>'''

out = '/home/elias/Downloads/keystrokes.html'
with open(out, 'w') as f:
    f.write(page)

print(f"Written: {out}")
