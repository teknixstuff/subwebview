Components.utils.import("resource://gre/modules/Services.jsm");

function selectMenuItem(menu, item) {
  setTimeout(()=>menu.selectedItem = menu.querySelector(`menuitem[value="${item}"]`));
}

function loadSettings() {
  let sitesList = document.getElementById('sites_list');
  let sitesTemplate = document.getElementById('sites_template');
  let sitesConfig;
  try {
    sitesConfig = JSON.parse(Services.prefs.getCharPref('extensions.subwebview.siteConfig'));
  } catch {
    sitesConfig = [];
  }
  let defaultEngine;
  for (const site of sitesConfig) {
    if (site.pattern == '') {
      defaultEngine = site.engine;
    } else {
      let siteItem = sitesTemplate.cloneNode(true);
      siteItem.removeAttribute('id');
      siteItem.removeAttribute('hidden');
      setTimeout(()=>siteItem.querySelector('.site_pattern').value = site.pattern);
      selectMenuItem(siteItem.querySelector('.site_engine'), site.engine);
      siteItem.classList.add('sites_item');
      sitesList.appendChild(siteItem);
    }
  }
  let sitesVoidItem = sitesTemplate.cloneNode(true);
  sitesVoidItem.id = 'sites_voidItem';
  sitesVoidItem.removeAttribute('hidden');
  if (defaultEngine) {
    selectMenuItem(sitesVoidItem.querySelector('.site_engine'), defaultEngine);
  }
  sitesList.appendChild(sitesVoidItem);
  document.getElementById('sites_header_engine').style.width = sitesVoidItem.querySelector('.site_engine').parentElement.getBoundingClientRect().width + 'px';
}

function patternChange(item) {
  if (item.id == 'sites_voidItem') {
    if (item.querySelector('.site_pattern').value) {
      item.removeAttribute('id');
      item.classList.add('sites_item');
      let sitesList = document.getElementById('sites_list');
      let sitesTemplate = document.getElementById('sites_template');
      let sitesVoidItem = sitesTemplate.cloneNode(true);
      sitesVoidItem.id = 'sites_voidItem';
      sitesVoidItem.removeAttribute('hidden');
      selectMenuItem(sitesVoidItem.querySelector('.site_engine'), item.querySelector('.site_engine').selectedItem.value);
      sitesList.appendChild(sitesVoidItem);
    }
  } else if (!item.querySelector('.site_pattern').value) {
    item.remove();
    document.getElementById('sites_voidItem').querySelector('.site_pattern').focus();
  }
}

function saveSettings() {
  let sitesList = document.getElementById('sites_list');
  let sitesConfig = [];
  sitesList.querySelectorAll('.sites_item, #sites_voidItem').forEach(siteItem=>{
    let site = {};
    site.pattern = siteItem.querySelector('.site_pattern').value;
    site.engine = siteItem.querySelector('.site_engine').selectedItem.value;
    sitesConfig.push(site);
  });
  Services.prefs.setCharPref('extensions.subwebview.siteConfig', JSON.stringify(sitesConfig));
  return true;
}