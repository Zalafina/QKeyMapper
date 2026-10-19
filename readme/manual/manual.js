(function () {
  'use strict';
  var articles = document.querySelectorAll('article'), nav = document.getElementById('nav');
  var search = document.getElementById('search'), results = document.getElementById('search-results');
  var status = document.getElementById('search-status'), lightbox = document.getElementById('lightbox');
  var full = document.getElementById('image-full'), viewport = lightbox.querySelector('.lightbox-viewport');
  var closeButton = document.getElementById('image-close'), fitButton = document.getElementById('image-fit');
  var previousFocus = null, index = [], timer;
  var themeButtons = document.querySelectorAll('[data-theme-choice]');
  function showTheme(theme) {
    document.documentElement.setAttribute('data-theme', theme);
    for (var j = 0; j < themeButtons.length; j++) themeButtons[j].setAttribute('aria-pressed', themeButtons[j].getAttribute('data-theme-choice') === theme ? 'true' : 'false');
  }
  showTheme(document.documentElement.getAttribute('data-theme'));
  for (var t = 0; t < themeButtons.length; t++) themeButtons[t].addEventListener('click', function () {
    var theme = this.getAttribute('data-theme-choice');
    showTheme(theme);
    try { window.localStorage.setItem('qkm-manual-theme', theme); } catch (e) { /* The current session still works without storage. */ }
  });
  function text(node) { return node.textContent || node.innerText || ''; }
  function normalize(value) { return value.toLowerCase().replace(/\s+/g, ''); }
  function loadImages(node) {
    var images = node.querySelectorAll('img[data-src]'), i;
    for (i = 0; i < images.length; i++) if (!images[i].getAttribute('src')) images[i].setAttribute('src', images[i].getAttribute('data-src'));
  }
  function route() {
    var id; try { id = decodeURIComponent(location.hash.slice(1)); } catch (e) { id = ''; }
    if (id === 'content') { document.getElementById('content').focus(); return; }
    var target = document.getElementById(id || 'quickstart'), article = target, i;
    while (article && article.tagName !== 'ARTICLE') article = article.parentNode;
    if (!article) { article = document.getElementById('quickstart'); target = article; }
    for (i = 0; i < articles.length; i++) articles[i].hidden = articles[i] !== article;
    loadImages(article);
    document.getElementById('location').textContent = article.getAttribute('data-title');
    document.title = article.getAttribute('data-title') + ' · QKeyMapper 使用手册';
    var links = document.querySelectorAll('.sidebar a');
    for (i = 0; i < links.length; i++) { if (links[i].getAttribute('href') === '#' + article.id) links[i].setAttribute('aria-current', 'page'); else links[i].removeAttribute('aria-current'); }
    if (target !== article) target.scrollIntoView(); else window.scrollTo(0, 0);
  }
  var sections = document.querySelectorAll('article > section'), i;
  for (i = 0; i < sections.length; i++) {
    var heading = sections[i].querySelector('h2,h3');
    index.push({id: sections[i].id, title: heading ? text(heading) : sections[i].parentNode.getAttribute('data-title'),
      chapter: sections[i].parentNode.getAttribute('data-title'), body: text(sections[i]).replace(/\s+/g, ' '),
      keywords: sections[i].getAttribute('data-search') || ''});
  }
  function runSearch() {
    var query = normalize(search.value), count = 0, j; results.innerHTML = '';
    if (!query) { results.hidden = true; nav.hidden = false; status.textContent = ''; return; }
    results.hidden = false; nav.hidden = true;
    var summary = document.createElement('p'); results.appendChild(summary);
    for (j = 0; j < index.length; j++) {
      var item = index[j];
      if (normalize(item.title + item.body + item.keywords).indexOf(query) < 0) continue;
      count++;
      var link = document.createElement('a'), title = document.createElement('strong'), snippet = document.createElement('small');
      link.href = '#' + item.id; title.textContent = item.title;
      var pos = item.body.toLowerCase().indexOf(search.value.toLowerCase());
      snippet.textContent = item.chapter + ' · ' + item.body.slice(Math.max(0, pos - 18), Math.max(0, pos - 18) + 82) + '…';
      link.appendChild(title); link.appendChild(snippet); results.appendChild(link);
    }
    summary.textContent = count ? '找到 ' + count + ' 处相关说明' : '没有找到。试试“原始按键”“停止映射”或更短的关键词。';
    status.textContent = summary.textContent;
  }
  search.addEventListener('input', function () { clearTimeout(timer); timer = setTimeout(runSearch, 100); });
  results.addEventListener('click', function (event) { var node = event.target; while (node && node.tagName !== 'A') node = node.parentNode; if (node) { search.value = ''; runSearch(); } });
  function closeImage() { lightbox.hidden = true; full.removeAttribute('src'); document.body.style.overflow = ''; if (previousFocus) previousFocus.focus(); }
  document.addEventListener('click', function (event) {
    var button = event.target;
    while (button && button !== document.body && !(button.tagName === 'BUTTON' && button.className.indexOf('image-button') >= 0)) button = button.parentNode;
    if (!button || button === document.body) return;
    var image = button.querySelector('img'); if (!image) return;
    previousFocus = button; full.src = image.getAttribute('data-src'); full.alt = image.alt;
    document.getElementById('image-title').textContent = image.alt;
    viewport.className = 'lightbox-viewport'; fitButton.textContent = '原尺寸';
    lightbox.hidden = false; document.body.style.overflow = 'hidden'; closeButton.focus();
  });
  closeButton.addEventListener('click', closeImage);
  fitButton.addEventListener('click', function () { var original = viewport.className.indexOf('original') < 0; viewport.className = original ? 'lightbox-viewport original' : 'lightbox-viewport'; fitButton.textContent = original ? '适应窗口' : '原尺寸'; });
  document.addEventListener('keydown', function (event) {
    var key = event.key || event.keyCode;
    if (key === 'Escape' || key === 27) { if (!lightbox.hidden) closeImage(); else if (search.value) {search.value = ''; runSearch();} }
    if (!lightbox.hidden && (key === 'Tab' || key === 9)) {
      if (event.shiftKey && document.activeElement === fitButton) { event.preventDefault(); closeButton.focus(); }
      else if (!event.shiftKey && document.activeElement === closeButton) { event.preventDefault(); fitButton.focus(); }
    }
    if ((key === '/' || key === 191) && !event.ctrlKey && !event.altKey && !event.metaKey && !/INPUT|TEXTAREA|SELECT/.test(document.activeElement.tagName)) {event.preventDefault(); search.focus();}
  });
  window.addEventListener('hashchange', route);
  window.addEventListener('beforeprint', function () { for (var j = 0; j < articles.length; j++) loadImages(articles[j]); });
  route();
}());
