"""Build the offline manual with Python's standard library only."""
import argparse
import base64
from html.parser import HTMLParser
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parent
MANUAL_VERSION = '0.7'
MANUAL_STATUS = '编写中'
SOFTWARE_BASELINE = 'v1.5.0.20261018'
REVIEW_DATE = '2026-10-19'

def favicon():
    data = (ROOT / 'images/QKeyMapper_icon_128px.png').read_bytes()
    if not data.startswith(b'\x89PNG\r\n\x1a\n'):
        raise ValueError('Manual icon must be a PNG')
    return 'data:image/png;base64,' + base64.b64encode(data).decode('ascii')

class DocumentCheck(HTMLParser):
    def __init__(self):
        super().__init__()
        self.ids = set()
        self.links = []
        self.images = 0
        self.icons = 0

    def handle_starttag(self, tag, attrs):
        attrs = dict(attrs)
        if 'id' in attrs:
            if attrs['id'] in self.ids:
                raise ValueError('Duplicate anchor: ' + attrs['id'])
            self.ids.add(attrs['id'])
        if tag == 'a' and attrs.get('href', '').startswith('#'):
            self.links.append(attrs['href'][1:])
        if tag == 'img' and ('data-src' in attrs or 'src' in attrs):
            source = attrs.get('data-src', attrs.get('src', ''))
            if not source.startswith('data:image/png;base64,') or not attrs.get('alt'):
                raise ValueError('Missing embedded PNG or image description')
            self.images += 1
        if tag == 'link' and attrs.get('rel') == 'icon':
            href = attrs.get('href', '')
            if attrs.get('type') != 'image/png' or not href.startswith('data:image/png;base64,'):
                raise ValueError('Favicon must be an embedded PNG')
            if not base64.b64decode(href.split(',', 1)[1], validate=True).startswith(b'\x89PNG\r\n\x1a\n'):
                raise ValueError('Invalid favicon PNG')
            self.icons += 1
        elif tag in ('script', 'link') and (attrs.get('src') or attrs.get('href')):
            raise ValueError('External script/stylesheet is not allowed')

def build(output):
    document = (ROOT / 'shell.html').read_text(encoding='utf-8')
    for token, name in [('STYLE', 'style.css'), ('SCRIPT', 'manual.js'), ('CONTENTS', 'chapters.html')]:
        document = document.replace('{{' + token + '}}', (ROOT / name).read_text(encoding='utf-8'))
    for token, value in [('MANUAL_VERSION', MANUAL_VERSION), ('MANUAL_STATUS', MANUAL_STATUS), ('SOFTWARE_BASELINE', SOFTWARE_BASELINE), ('REVIEW_DATE', REVIEW_DATE)]:
        document = document.replace('{{' + token + '}}', value)
    document = document.replace('{{FAVICON}}', favicon())
    total = 0
    def embed(match):
        nonlocal total
        path = (ROOT / match.group(2)).resolve()
        if not path.is_relative_to(ROOT / 'images'):
            raise ValueError('Image must be inside images/: ' + str(path))
        data = path.read_bytes()
        if not data.startswith(b'\x89PNG\r\n\x1a\n'):
            raise ValueError('Not a PNG: ' + str(path))
        total += len(data)
        return match.group(1) + '="data:image/png;base64,' + base64.b64encode(data).decode('ascii') + '"'
    document = re.sub(r'(data-src|src)="(images/[^"<>]+\.png)"', embed, document)
    check = DocumentCheck()
    check.feed(document)
    if check.icons != 1:
        raise ValueError('Exactly one embedded favicon is required')
    missing = set(check.links) - check.ids
    if missing:
        raise ValueError('Broken internal links: ' + ', '.join(sorted(missing)))
    if re.search(r'\{\{(?:STYLE|CONTENTS|SCRIPT|MANUAL_VERSION|MANUAL_STATUS|SOFTWARE_BASELINE|REVIEW_DATE|FAVICON)\}\}', document):
        raise ValueError('Unresolved template')
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open('w', encoding='utf-8', newline='\n') as target:
        target.write(document)
    print(f'{output}\nHTML: {output.stat().st_size:,} bytes; embedded PNG: {total:,} bytes; image references: {check.images}; anchors: {len(check.ids)}')

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT.parents[1] / 'out/user_manual/release/QKeyMapper使用手册.html')
    build(parser.parse_args().output.resolve())
