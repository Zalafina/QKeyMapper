/* Offline checks for selected manual chapters; no browser downloads or installs. */
const fs = require('fs');
const path = require('path');
const assert = require('assert');
const {pathToFileURL} = require('url');

function options(argv) {
  const out = {channels: ['chrome', 'msedge'], chapters: ['quickstart'], queries: []};
  for (let i = 0; i < argv.length; i++) {
    const name = argv[i];
    if (name === '--help') { out.help = true; continue; }
    if (!['--file', '--output', '--module', '--channels', '--chapters', '--query'].includes(name) || !argv[i + 1]) throw Error('Unknown or incomplete option: ' + name);
    const value = argv[++i];
    if (name === '--query') out.queries.push(value);
    else if (name === '--chapters' || name === '--channels') out[name.slice(2)] = value.split(',');
    else out[name.slice(2)] = value;
  }
  const repo = path.resolve(__dirname, '../../../..');
  out.file = path.resolve(out.file || path.join(repo, 'out/user_manual/release/QKeyMapper使用手册.html'));
  out.output = path.resolve(out.output || path.join(repo, 'out/user_manual/browser_check'));
  return out;
}

async function run(opt) {
  const {chromium} = require(opt.module || process.env.QKM_PLAYWRIGHT_MODULE || 'playwright');
  fs.mkdirSync(opt.output, {recursive: true});
  const isolated = path.join(opt.output, 'isolated');
  fs.mkdirSync(isolated, {recursive: true});
  const file = path.join(isolated, path.basename(opt.file));
  fs.copyFileSync(opt.file, file);
  const reports = [];
  for (const channel of opt.channels) {
    assert(['chrome', 'msedge'].includes(channel), 'Use an installed Chrome or Edge');
    const browser = await chromium.launch({channel, headless: true});
    try {
      const context = await browser.newContext({viewport: {width: 1440, height: 1000}, offline: true});
      const page = await context.newPage();
      const errors = [], external = [];
      page.on('pageerror', e => errors.push(e.message));
      page.on('request', r => { if (!/^(file:|data:)/.test(r.url())) external.push(r.url()); });
      await page.goto(pathToFileURL(file).href);
      for (const theme of ['dark', 'light']) {
        await page.locator(`[data-theme-choice="${theme}"]`).click();
        assert.strictEqual(await page.locator('html').getAttribute('data-theme'), theme);
        for (const chapter of opt.chapters) {
          assert(/^[a-zA-Z0-9_-]+$/.test(chapter), 'Use a chapter or section id');
          const nav = page.locator(`#nav a[href="#${chapter}"]`);
          if (await nav.count()) await nav.click();
          else await page.evaluate(id => { location.hash = id; }, chapter);
          await page.locator('#' + chapter).waitFor({state: 'visible'});
          await page.waitForFunction(id => Array.from(document.getElementById(id).querySelectorAll('img')).every(img => img.complete && img.naturalWidth > 0), chapter);
          await page.locator('#' + chapter).scrollIntoViewIfNeeded();
          await page.screenshot({path: path.join(opt.output, `${channel}_${theme}_${chapter}.png`)});
          await page.setViewportSize({width: 390, height: 844});
          await page.locator('#' + chapter).scrollIntoViewIfNeeded();
          assert(await page.evaluate(() => document.documentElement.scrollWidth <= innerWidth + 1), 'Narrow overflow: ' + chapter);
          await page.screenshot({path: path.join(opt.output, `${channel}_${theme}_${chapter}_narrow.png`)});
          await page.setViewportSize({width: 1440, height: 1000});
        }
      }
      for (const term of opt.queries) {
        await page.locator('#search').fill(term);
        await page.locator('#search-results a').first().waitFor({state: 'visible'});
        await page.locator('#search-results a').first().click();
        assert(await page.locator('article:visible').count() === 1, 'Search must reach an article');
      }
      await page.locator('#search').fill('');
      await page.evaluate(id => { location.hash = id; }, opt.chapters[0]);
      await page.locator('#' + opt.chapters[0]).waitFor({state: 'visible'});
      const image = page.locator('article:visible .image-button').first();
      let imageChecked = false;
      if (await image.count()) {
        await image.click();
        await page.locator('#lightbox').waitFor({state: 'visible'});
        await page.waitForFunction(() => { const img = document.getElementById('image-full'); return img.complete && img.naturalWidth > 0; });
        await page.keyboard.press('Escape');
        assert(await page.locator('#lightbox').isHidden());
        imageChecked = true;
      }
      assert.deepStrictEqual(errors, []);
      assert.deepStrictEqual(external, []);
      reports.push({channel, browser: browser.version(), chapters: opt.chapters, queries: opt.queries, imageChecked, errors, external});
      await context.close();
    } finally { await browser.close(); }
  }
  fs.writeFileSync(path.join(opt.output, 'results.json'), JSON.stringify(reports, null, 2) + '\n');
  console.log(JSON.stringify(reports, null, 2));
}
if (require.main === module) {
  try {
    const opt = options(process.argv.slice(2));
    if (opt.help) console.log('node check_browser.cjs [--file HTML] [--output DIR] [--module PLAYWRIGHT_PATH] [--channels chrome,msedge] [--chapters quickstart,table-filter] [--query 搜索词]');
    else run(opt).catch(error => { console.error(error); process.exitCode = 1; });
  } catch (error) { console.error(error); process.exitCode = 1; }
}
module.exports = {options};
