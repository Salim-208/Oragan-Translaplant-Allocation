// Dashboard script: loads /api/data, draws the tables, and sends button/form actions to the C++ API.
(() => {
  const $ = s => document.querySelector(s);
  const $$ = s => document.querySelectorAll(s);
  const BLOOD = ['O-', 'O+', 'A-', 'A+', 'B-', 'B+', 'AB-', 'AB+'];
  const THEME_KEY = 'organ-allocation-theme';
  const EXPIRY_ALERT_SECONDS = 24 * 60 * 60;
  const expiryAlertsShown = new Set();
  let data = null;
  let auth = JSON.parse(localStorage.getItem('organ-auth') || 'null');
  const routeMaps = new Map();
  let hospitalTransferCache = [];

  // ---------- small helpers ----------
  const esc = v => String(v ?? '—').replace(/[&<>"']/g, c => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c]));
  const f = key => row => esc(row[key]);                                  // column: show one field
  const reference = (prefix, id) => id === null || id === undefined || id === '' ? '—' : `${prefix}-TR-${String(id).padStart(5, '0')}`;
  const idCell = (prefix, id) => `<span class="record-id">${esc(reference(prefix, id))}</span>`;
  const badge = v => `<span class="badge badge-${esc(v)}">${esc(v)}</span>`;
  const date = v => (v ? new Date(v).toLocaleString() : '—');
  const remaining = seconds => {
    const totalMinutes = Math.ceil(Math.max(0, Number(seconds || 0)) / 60);
    const hours = Math.floor(totalMinutes / 60);
    const minutes = totalMinutes % 60;
    return `${hours}h ${minutes}m`;
  };
  const button = (label, attrs) => `<button class="small-action" ${attrs}>${label}</button>`;
  const decisionButtons = (kind, row) => {
    if (row.status === 'ALLOCATED') return '<span class="tag">Allocated</span>';
    if (kind === 'offer' && row.status === 'ACCEPTED') return '<span class="tag">Added to organ queue</span>';
    if (kind === 'request' && row.status === 'ACCEPTED') return '<span class="tag">Approved · awaiting allocation</span>';
    const approve = kind === 'request' ? 'Approve' : 'Verify & add organ';
    return `${button(approve, `data-decision="${kind}" data-id="${row[kind === 'request' ? 'request_id' : 'offer_id']}" data-status="ACCEPTED"`)} ${button('Reject', `data-decision="${kind}" data-id="${row[kind === 'request' ? 'request_id' : 'offer_id']}" data-status="REJECTED"`)} ${button('Pending', `data-decision="${kind}" data-id="${row[kind === 'request' ? 'request_id' : 'offer_id']}" data-status="PENDING"`)}`;
  };

  // table(columns, rows, emptyText): a column is [HEADER, row => html]
  const table = (cols, rows, empty) => rows.length
    ? `<table><thead><tr>${cols.map(c => `<th>${c[0]}</th>`).join('')}</tr></thead><tbody>` +
      rows.map(r => `<tr>${cols.map(c => `<td>${c[1](r)}</td>`).join('')}</tr>`).join('') + '</tbody></table>'
    : `<div class="empty">${esc(empty)}</div>`;

  async function api(url, options = {}) {
    const headers = options.body ? { 'Content-Type': 'application/json' } : {};
    if (auth?.token) headers.Authorization = `Bearer ${auth.token}`;
    const response = await fetch(url, { cache: 'no-store', ...options, headers: { ...headers, ...options.headers } });
    const result = await response.json().catch(() => ({}));
    if (!response.ok) { const error=new Error(result.error || `Request failed (${response.status})`); error.status=response.status; throw error; }
    return result;
  }
  const post = (url, body) => api(url, { method: 'POST', body: body && JSON.stringify(body) });

  function notify(message, error = false) {
    const box = $('#notice');
    box.textContent = message;
    box.hidden = false;
    box.classList.toggle('error', error);
    clearTimeout(notify.timer);
    notify.timer = setTimeout(() => { box.hidden = true; }, 5000);
  }

  function routeMap(mapId) {
    if (!window.L) throw new Error('Map library did not load. Check your internet connection and refresh.');
    if (routeMaps.has(mapId)) return routeMaps.get(mapId);
    const map=L.map(mapId,{scrollWheelZoom:false}).setView([30.1,79.0],7);
    L.tileLayer('https://tile.openstreetmap.org/{z}/{x}/{y}.png',{maxZoom:19,attribution:'&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors'}).addTo(map);
    routeMaps.set(mapId,{map,layer:null,markers:[]});
    return routeMaps.get(mapId);
  }

  function showHospitalNetwork(hospitals) {
    if(routeMaps.has('admin-route-map')) return;
    let state; const bounds=[];
    try { state=routeMap('admin-route-map'); }
    catch(error) { document.getElementById('admin-route-map').textContent=error.message; return; }
    for(const hospital of hospitals) {
      const lat=Number(hospital.latitude),lon=Number(hospital.longitude);
      if(!Number.isFinite(lat)||!Number.isFinite(lon)) continue;
      const point=[lat,lon]; bounds.push(point);
      state.markers.push(L.marker(point,{icon:L.divIcon({className:'route-marker network-marker',html:'H',iconSize:[26,26],iconAnchor:[13,13]})}).addTo(state.map)
        .bindPopup(`<b>${esc(hospital.name)}</b><br>${esc(hospital.city||'')}<br>Approximate training location`));
    }
    if(bounds.length) state.map.fitBounds(L.latLngBounds(bounds),{padding:[24,24],maxZoom:8});
    requestAnimationFrame(()=>state.map.invalidateSize());
  }

  async function showHospitalRoute(mapId,summaryId,from,to) {
    const summary=document.getElementById(summaryId);
    summary.hidden=false;
    summary.innerHTML='<div class="route-metric"><span>Route status</span><strong>Calculating road route…</strong></div>';
    const a={lat:Number(from.latitude),lon:Number(from.longitude)},b={lat:Number(to.latitude),lon:Number(to.longitude)};
    if (![a.lat,a.lon,b.lat,b.lon].every(Number.isFinite)) throw new Error('Hospital location coordinates are missing.');
    const directKm=6371*2*Math.asin(Math.sqrt(Math.sin((b.lat-a.lat)*Math.PI/360)**2+Math.cos(a.lat*Math.PI/180)*Math.cos(b.lat*Math.PI/180)*Math.sin((b.lon-a.lon)*Math.PI/360)**2));
    let state;
    try { state=routeMap(mapId); }
    catch(error) { summary.innerHTML=`<div class="route-alert">${esc(error.message)}</div>`; return; }
    if(state.layer) state.map.removeLayer(state.layer);
    state.markers.forEach(marker=>state.map.removeLayer(marker)); state.markers=[];
    const fromPoint=[a.lat,a.lon],toPoint=[b.lat,b.lon];
    const marker=(point,letter,cls,label)=>L.marker(point,{icon:L.divIcon({className:`route-marker ${cls}`,html:letter,iconSize:[28,28],iconAnchor:[14,14]})}).addTo(state.map).bindPopup(`<b>${esc(label)}</b><br>${letter==='D'?'Donor hospital':'Recipient hospital'}`);
    state.markers.push(marker(fromPoint,'D','donor-marker',`${from.name} (${from.city||''})`),marker(toPoint,'R','recipient-marker',`${to.name} (${to.city||''})`));
    let routeData=null;
    try {
      const url=`https://router.project-osrm.org/route/v1/driving/${a.lon},${a.lat};${b.lon},${b.lat}?overview=full&geometries=geojson&steps=false`;
      const response=await fetch(url); if(!response.ok) throw new Error(`Routing service returned ${response.status}`);
      const payload=await response.json(); if(payload.code!=='Ok'||!payload.routes?.length) throw new Error('No drivable route was found between these locations.');
      routeData=payload.routes[0];
      state.layer=L.geoJSON(routeData.geometry,{style:{color:'#1976c9',weight:5,opacity:.82}}).addTo(state.map);
    } catch(error) {
      state.layer=L.polyline([fromPoint,toPoint],{color:'#7c8791',weight:4,dashArray:'8 8'}).addTo(state.map);
      summary.innerHTML=`<div class="route-metric"><span>Route status</span><strong>Road routing unavailable</strong></div><div class="route-metric"><span>Straight-line distance</span><strong>${directKm.toFixed(1)} km</strong></div><div class="route-metric"><span>Travel time</span><strong>Unavailable</strong></div><div class="route-alert">${esc(error.message)} The dashed line is a direct map connection, not a road route.</div>`;
    }
    if(routeData) {
      const km=routeData.distance/1000,duration=routeData.duration,arrival=new Date(Date.now()+duration*1000).toLocaleString();
      summary.innerHTML=`<div class="route-metric"><span>Road distance</span><strong>${km.toFixed(1)} km</strong></div><div class="route-metric"><span>Estimated drive time</span><strong>${remaining(duration)} </strong></div><div class="route-metric"><span>ETA if departing now</span><strong>${esc(arrival)}</strong></div><div class="route-alert">${esc(from.name)} → ${esc(to.name)} · OSRM road estimate; live traffic and vehicle location are not included.</div>`;
    }
    const bounds=L.latLngBounds([fromPoint,toPoint]); state.map.fitBounds(state.layer.getBounds().isValid()?state.layer.getBounds():bounds,{padding:[32,32],maxZoom:12});
    requestAnimationFrame(()=>state.map.invalidateSize());
  }

  // ---------- table columns ----------
  const COLS = {
    queue: [['ORGAN ID', o => idCell('ORG', o.organ_id)], ['ORGAN', f('organ_type')], ['BLOOD', f('blood_type')], ['STATUS', o => badge(o.status)],
      ['TIME LEFT', o => remaining(o.seconds_remaining)],
      ['QUEUE', o => button('Rank recipients', `data-match="${esc(o.organ_id)}"`)]],
    allocations: [['ALLOCATION ID', a => idCell('ALC', a.allocation_id)], ['ORGAN', a => `${esc(a.organ_type)} · ${reference('ORG', a.organ_id)}`], ['PATIENT ID', a => idCell('PAT', a.recipient_id)], ['RECIPIENT', f('recipient_name')], ['HOSPITAL', f('hospital_name')],
      ['STATUS', a => badge(a.status)], ['ALLOCATED AT', a => date(a.allocated_at)]],
    hospitals: [['HOSPITAL ID', h => idCell('HSP', h.hospital_id)], ['HOSPITAL', f('name')], ['CITY', f('city')], ['ADDRESS', f('address')], ['CONTACT', f('phone')], ['APPROX. LOCATION', h => `${esc(h.latitude)}, ${esc(h.longitude)}`]],
    donors: [['DONOR ID', d => idCell('DNR', d.donor_id)], ['DONOR', f('name')], ['AGE', f('age')], ['BLOOD', f('blood_type')], ['TISSUE PROFILE', f('tissue_type')], ['HEALTH NOTES', f('health_notes')],
      ['TRAINING FLAG', d => (d.medically_eligible ? 'Eligible in simulation' : 'Ineligible in simulation')]],
    organs: [['ORGAN ID', o => idCell('ORG', o.organ_id)], ['ORGAN', f('organ_type')], ['DONOR ID', o => idCell('DNR', o.donor_id)], ['BLOOD', f('blood_type')],
      ['STATUS', o => badge(o.status)], ['EXPIRES', o => date(o.expires_at)], ['VIABILITY NOTES', f('viability_notes')]],
    recipients: [['PATIENT ID', r => idCell('PAT', r.recipient_id)], ['RECIPIENT', f('name')], ['AGE', f('age')], ['ORGAN', f('organ_type')], ['BLOOD', f('blood_type')], ['TISSUE PROFILE', f('tissue_type')],
      ['URGENCY', r => `${esc(r.urgency_score)}/10`], ['WAIT', r => `${esc(r.waiting_days)} days`], ['HOSPITAL', f('hospital_name')], ['COMPATIBILITY', f('compatibility_notes')], ['MEDICAL NOTES', f('medical_notes')]],
    events: [['EVENT', e => badge(e.event_type)], ['ENTITY', f('entity_type')], ['REFERENCE', e => idCell(({organ:'ORG',request:'REQ',offer:'OFF',system:'SYS'})[e.entity_type] || 'REC', e.entity_id)],
      ['DETAIL', e => esc(e.payload?.reason || e.payload?.description || JSON.stringify(e.payload || {}))], ['TIME', e => date(e.created_at)]],
    requests: [['REQUEST ID', q => idCell('REQ', q.request_id)], ['PATIENT ID', q => idCell('PAT', q.recipient_id)], ['HOSPITAL', f('hospital_name')], ['HOSPITAL PATIENT REF', f('patient_ref')], ['AGE', f('patient_age')], ['ORGAN', f('organ_type')], ['BLOOD', f('blood_type')], ['TISSUE PROFILE', f('tissue_type')], ['URGENCY', q => `${esc(q.urgency_score)}/10`], ['COMPATIBILITY', f('compatibility_notes')], ['MEDICAL NOTES', f('medical_notes')], ['STATUS', q => badge(q.status)], ['DECISION', q => decisionButtons('request', q)]],
    offers: [['OFFER ID', q => idCell('OFF', q.offer_id)], ['HOSPITAL', f('hospital_name')], ['DONOR REF', f('donor_ref')], ['ORGAN', f('organ_type')], ['BLOOD', f('blood_type')], ['TISSUE PROFILE', f('tissue_type')], ['HEALTH NOTES', f('health_notes')], ['AVAILABLE', q => date(q.available_at)], ['EXPIRES', q => date(q.expires_at)], ['STATUS', q => badge(q.status)], ['INVENTORY ORGAN', q => q.organ_id ? idCell('ORG', q.organ_id) : '—'], ['DECISION', q => decisionButtons('offer', q)]],
    notifications: [['HOSPITAL ID', q => idCell('HSP', q.hospital_id)], ['UPDATE', f('title')], ['DETAIL', f('message')], ['TIME', q => date(q.created_at)]],
    matches: organId => [['RANK', m => m.rank], ['PATIENT ID', m => idCell('PAT', m.recipient_id)], ['RECIPIENT', m => `${esc(m.name)}<br><small>${esc(m.hospital_name)}</small>`], ['AGE', f('age')],
      ['BLOOD', f('blood_type')], ['TISSUE', f('tissue_type')], ['PROFILE EQUALITY', m => m.tissue_profile_match ? 'Exact training profile' : 'Manual review'], ['URGENCY', f('urgency_score')], ['WAIT', m => `${esc(m.waiting_days)}d`],
      ['DISTANCE', m => `${Number(m.distance_km).toFixed(1)} km`], ['SCORE', m => Number(m.priority_score).toFixed(1)],
      ['ACTION', m => button('Allocate', `data-claim-organ="${organId}" data-claim-recipient="${m.recipient_id}"`)]],
  };

  // ---------- drawing ----------
  function render() {
    const { analytics: a, organs, allocations, hospitals, donors, recipients, events, requests, offers, notifications } = data;
    const demo = a.demo_counts || {};
    const modules = ['hospitals', 'donors', 'organs', 'recipients'];
    const loaded = modules.every(key => Number(demo[key] || 0) === 14);
    $('#demo-progress').textContent = loaded ? '14 fictional records per core table' : 'Training dataset not loaded';
    $('#seed').disabled = false;

    $('#metrics').innerHTML = [['Available organs', a.available], ['Allocated', a.allocated], ['Wasted', a.wasted],
      ['C++ server process', $('#metrics').dataset.pid || '—']]
      .map(([name, value]) => `<article class="metric"><span>${esc(name)}</span><strong>${esc(value ?? 0)}</strong></article>`).join('');
    const rate = a.total ? Math.round(Number(a.allocated) * 100 / Number(a.total)) : 0;
    $('#analytics').innerHTML = [['Allocation rate', `${rate}%`], ['Average recipient wait', `${esc(a.average_wait_days)} days`], ['Total organs', esc(a.total)]]
      .map(([name, value]) => `<div class="analytics-row"><span>${name}</span><b>${value}</b></div>`).join('');

    const availableOrgans = organs.filter(o => o.status === 'AVAILABLE')
      .sort((a, b) => Number(a.seconds_remaining) - Number(b.seconds_remaining));
    $('#organs-table').innerHTML = table(COLS.queue, availableOrgans, 'No available organs. Replace the dataset with fictional training records to populate this queue.');
    const soonest = availableOrgans[0];
    if (soonest && Number(soonest.seconds_remaining) <= EXPIRY_ALERT_SECONDS && !expiryAlertsShown.has(String(soonest.organ_id)) && !$('#expiry-dialog').open) {
      expiryAlertsShown.add(String(soonest.organ_id));
      $('#expiry-alert-title').textContent = `${soonest.organ_type} ${reference('ORG', soonest.organ_id)} is expiring soon`;
      $('#expiry-alert-message').textContent = `Only ${remaining(soonest.seconds_remaining)} remain. Review matches or allocate now.`;
      $('#expiry-view-matches').dataset.organId = soonest.organ_id;
      $('#expiry-priority-allocate').dataset.organId = soonest.organ_id;
      $('#expiry-dialog').showModal();
    }
    $('#allocations-table').innerHTML = table(COLS.allocations, allocations, 'No allocation decisions yet.');
    $('#hospitals-table').innerHTML = table(COLS.hospitals, hospitals, 'No hospitals.');
    $('#donors-table').innerHTML = table(COLS.donors, donors, 'No donors.');
    $('#all-organs-table').innerHTML = table(COLS.organs, organs, 'No organs.');
    $('#recipients-table').innerHTML = table(COLS.recipients, recipients, 'No recipients.');
    $('#events-table').innerHTML = table(COLS.events, events, 'No events recorded.');
    $('#requests-table').innerHTML = table(COLS.requests, requests || [], 'No organ requests submitted.');
    $('#offers-table').innerHTML = table(COLS.offers, offers || [], 'No donation offers submitted.');
    $('#notifications-table').innerHTML = table(COLS.notifications, notifications || [], 'No decision notifications.');
    fillOptions();
    showHospitalNetwork(hospitals);
  }

  // dropdowns in the forms
  function fillOptions() {
    const options = (items, value, label) => items.map(x => `<option value="${x[value]}">${label(x)}</option>`).join('');
    const hospitals = options(data.hospitals, 'hospital_id', h => esc(h.name));
    $$('select[name="blood_type"]').forEach(s => { s.innerHTML = BLOOD.map(b => `<option>${b}</option>`).join(''); });
    $('#recipient-hospital').innerHTML = hospitals || '<option value="">Add a hospital first</option>';
    $('#organ-hospital').innerHTML = '<option value="">Not specified</option>' + hospitals;
    $$('.hospital-options').forEach(s => { s.innerHTML = options(data.hospitals, 'hospital_id', h => `${reference('HSP', h.hospital_id)} · ${esc(h.name)}`); });
    $('#donor-select').innerHTML = options(data.donors, 'donor_id', d => `${reference('DNR', d.donor_id)} · ${esc(d.name)} (${esc(d.blood_type)})`) || '<option value="">Add a donor first</option>';
    $$('.organ-options').forEach(s => { s.innerHTML = options(data.organs, 'organ_id', o => `${reference('ORG', o.organ_id)} · ${esc(o.organ_type)}`); });
    const routeOpts=options(data.hospitals,'hospital_id',h=>`${esc(h.name)} · ${esc(h.city)}`);
    for(const selector of ['#route-from','#route-to']) {
      const select=$(selector), previous=select.value;
      select.innerHTML=routeOpts;
      if(data.hospitals.some(h=>String(h.hospital_id)===previous)) select.value=previous;
    }
    if(data.hospitals.length>1 && $('#route-from').value===$('#route-to').value) $('#route-to').selectedIndex=1;
  }

  async function load() {
    if (!auth) return false;
    try {
      if (auth.role === 'hospital') {
        const d = await api('/api/hospital/data');
        $('#hospital-title').textContent = `${d.hospital.name} dashboard`;
        $('#my-requests').innerHTML = table([['REQUEST ID',q=>idCell('REQ',q.request_id)],['PATIENT ID',q=>idCell('PAT',q.recipient_id)],['HOSPITAL PATIENT REF',f('patient_ref')],['AGE',f('patient_age')],['ORGAN',f('organ_type')],['BLOOD',f('blood_type')],['TISSUE PROFILE',f('tissue_type')],['URGENCY',f('urgency_score')],['STATUS',q=>badge(q.status)],['DECISION',f('decision_reason')]],d.requests,'No requests yet.');
        $('#my-offers').innerHTML = table([['OFFER ID',q=>idCell('OFF',q.offer_id)],['DONOR REF',f('donor_ref')],['ORGAN',f('organ_type')],['BLOOD',f('blood_type')],['TISSUE PROFILE',f('tissue_type')],['EXPIRES',q=>date(q.expires_at)],['STATUS',q=>badge(q.status)],['ORGAN QUEUE ID',q=>q.organ_id?idCell('ORG',q.organ_id):'—'],['DECISION',f('decision_reason')]],d.offers,'No donation offers yet.');
        $('#my-notifications').innerHTML = table([['UPDATE',f('title')],['DETAIL',f('message')],['TIME',q=>date(q.created_at)]],d.notifications,'No notifications yet.');
        hospitalTransferCache=d.transfers||[];
        $('#hospital-transfers').innerHTML=table([
          ['ALLOCATION',t=>idCell('ALC',t.allocation_id)],['ORGAN',t=>`${esc(t.organ_type)} · ${reference('ORG',t.organ_id)}`],
          ['DONOR HOSPITAL',f('donor_hospital_name')],['RECIPIENT HOSPITAL',f('recipient_hospital_name')],['ALLOCATED',t=>date(t.allocated_at)],
          ['ROUTE',t=>button('View route',`data-hospital-transfer="${hospitalTransferCache.indexOf(t)}"`)]
        ],hospitalTransferCache,'No approved organ transfers involve this hospital yet.');
        $('#hospital-route-map').hidden=hospitalTransferCache.length===0;
        $('#connection').className = 'connection online'; $('#connection').innerHTML = '<i></i> Hospital workspace online';
        return true;
      }
      const [d, system] = await Promise.all([api('/api/data'), api('/api/status')]);
      data = d;
      $('#metrics').dataset.pid = system.pid;
      $('#connection').className = 'connection online';
      $('#connection').innerHTML = `<i></i> Online · PID ${system.pid}`;
      render();
      return true;
    } catch (e) {
      if (e.status === 401 || e.status === 403) {
        auth=null; localStorage.removeItem('organ-auth');
        $('#my-requests').innerHTML=''; $('#my-offers').innerHTML=''; $('#my-notifications').innerHTML='';
        workspace();
      }
      $('#connection').className = 'connection offline';
      $('#connection').innerHTML = '<i></i> Offline';
      notify(e.message, true);
      return false;
    }
  }

  function showView(view) {
    $$('.view').forEach(x => x.classList.toggle('active', x.id === `${view}-view`));
    $$('[data-view]').forEach(x => x.classList.toggle('active', x.dataset.view === view));
  }

  function workspace() {
    const signedIn = Boolean(auth?.token);
    const isAdmin = signedIn && auth.role === 'admin';
    const isHospital = signedIn && auth.role === 'hospital';
    $('#login-view').hidden = signedIn;
    $('#overview-view').hidden = !isAdmin;
    $('#overview-view').classList.toggle('active', isAdmin);
    $('#records-view').hidden = !isAdmin;
    $('#records-view').classList.remove('active');
    $('#systems-view').hidden = !isAdmin;
    $('#systems-view').classList.remove('active');
    $('#hospital-view').hidden = !isHospital;
    $('#hospital-view').classList.toggle('active', isHospital);
    document.querySelectorAll('[data-view]').forEach(x => x.hidden = !isAdmin);
    $('#theme').hidden = !signedIn;
    document.querySelectorAll('.logout').forEach(x => x.hidden = !signedIn);
    if (signedIn) load();
  }

  async function showMatches(organId) {
    const result = await api(`/api/organs/${organId}/matches`);
    $('#match-title').textContent = `${result.organ.organ_type} ${reference('ORG', result.organ.organ_id)}`;
    $('#match-table').innerHTML = table(COLS.matches(organId), result.matches, 'No compatible active recipients.');
    $('#auto-allocate').dataset.organId = organId;
    $('#auto-allocate').disabled = result.matches.length === 0;
    $('#match-dialog').showModal();
  }

  async function priorityAllocate(organId) {
    const result = await post('/api/auto-allocate', { organ_id: Number(organId) });
    if ($('#expiry-dialog').open) $('#expiry-dialog').close();
    if ($('#match-dialog').open) $('#match-dialog').close();
    await load();
    notify(`Priority scheduler allocated ${reference('PAT', result.recipient_id)} (${result.recipient_name}) to ${reference('ORG', result.organ_id)} (OS priority ${result.os_priority}).`);
  }

  function renderProcessSchedule(result) {
    const total = result.timeline.at(-1)?.end || 1;
    $('#schedule-gantt').innerHTML = result.timeline.map(s => {
      const width = Math.max(5, (s.end - s.start) * 100 / total);
      return `<div class="schedule-slice" style="width:${width}%"><b>${esc(s.process)}</b><small>${s.start}–${s.end}</small></div>`;
    }).join('');
    $('#schedule-averages').innerHTML = [
      ['Algorithm', result.algorithm === 'RR' ? `Round Robin (q=${result.quantum})` : result.algorithm],
      ['Average waiting time', `${Number(result.average_waiting_time).toFixed(2)} units`],
      ['Average turnaround time', `${Number(result.average_turnaround_time).toFixed(2)} units`],
    ].map(([label, value]) => `<div class="analytics-row"><span>${esc(label)}</span><b>${esc(value)}</b></div>`).join('');
    $('#schedule-results').innerHTML = table([
      ['PROCESS', f('id')], ['ARRIVAL', f('arrival_time')], ['BURST', f('burst_time')], ['PRIORITY', f('priority')],
      ['COMPLETION', f('completion_time')], ['WAITING', f('waiting_time')], ['TURNAROUND', f('turnaround_time')],
    ], result.results, 'No processes.');
    $('#process-schedule-output').hidden = false;
  }

  // ---------- button actions: [css selector, handler(element, event)] ----------
  const runPost = (path, label) => async () => {   // POST, refresh, show the result
    const r = await post(path);
    await load();
    notify(`${label} ${JSON.stringify(r)}`);
  };
  const ACTIONS = [
    ['[data-view]', (el, ev) => { ev.preventDefault(); showView(el.dataset.view); }],
    ['#refresh', async el => {
      el.disabled = true;
      el.textContent = 'Refreshing…';
      try {
        if (await load()) notify('Dashboard data refreshed.');
      } finally {
        el.disabled = false;
        el.textContent = 'Refresh';
      }
    }],
    ['#calculate-route', async () => {
      const from=data?.hospitals.find(h=>String(h.hospital_id)===$('#route-from').value);
      const to=data?.hospitals.find(h=>String(h.hospital_id)===$('#route-to').value);
      if(!from||!to) throw new Error('Choose both hospitals.');
      if(String(from.hospital_id)===String(to.hospital_id)) throw new Error('Choose two different hospitals.');
      $('#admin-route-map').hidden=false;
      await showHospitalRoute('admin-route-map','admin-route-summary',from,to);
    }],
    ['[data-hospital-transfer]', async el => {
      const transfer=hospitalTransferCache[Number(el.dataset.hospitalTransfer)];
      if(!transfer) throw new Error('This approved transfer is no longer available. Refresh the page.');
      const from={name:transfer.donor_hospital_name,city:transfer.donor_city,latitude:transfer.donor_latitude,longitude:transfer.donor_longitude};
      const to={name:transfer.recipient_hospital_name,city:transfer.recipient_city,latitude:transfer.recipient_latitude,longitude:transfer.recipient_longitude};
      $('#hospital-route-map').hidden=false;
      await showHospitalRoute('hospital-route-map','hospital-route-summary',from,to);
    }],
    ['#close-match', () => $('#match-dialog').close()],
    ['#theme', () => localStorage.setItem(THEME_KEY, document.body.classList.toggle('dark-mode') ? 'dark' : 'light')],
    ['.logout', async () => { try { await post('/api/logout'); } catch (_) {} auth=null; localStorage.removeItem('organ-auth'); workspace(); }],

    ['[data-match]', async el => {                  // rank recipients for one organ
      await showMatches(el.dataset.match);
    }],
    ['#auto-allocate', el => priorityAllocate(el.dataset.organId)],
    ['#expiry-view-matches', async el => { $('#expiry-dialog').close(); await showMatches(el.dataset.organId); }],
    ['#expiry-priority-allocate', el => priorityAllocate(el.dataset.organId)],
    ['#expiry-later', () => $('#expiry-dialog').close()],
    ['[data-claim-organ]', async el => {            // allocate: one database transaction on the server
      await post('/api/claim', { organ_id: Number(el.dataset.claimOrgan), recipient_id: Number(el.dataset.claimRecipient) });
      $('#match-dialog').close();
      await load();
      notify('Allocation committed.');
    }],
    ['[data-decision]', async el => {
      const reason = el.dataset.status === 'REJECTED' ? window.prompt('Enter the rejection reason (required):') : '';
      if (el.dataset.status === 'REJECTED' && !reason?.trim()) { if (reason !== null) notify('Rejection reason is required.', true); return; }
      await post('/api/decision', { kind: el.dataset.decision, id: Number(el.dataset.id), status: el.dataset.status, reason: reason || '' });
      await load(); notify(`${el.dataset.decision} marked ${el.dataset.status.toLowerCase()}; hospital notified.`);
    }],
    ['#seed', async () => {
      if (!window.confirm('This permanently deletes all current hospital, donor, recipient, organ, request, offer, allocation, notification, distance, and audit data, then installs the fictional training dataset. Continue?')) return;
      const r = await post('/api/seed');
      await load();
      notify(r.message || 'Training dataset replaced.');
    }],
    ['#deadlock-demo, #deadlock-lab', async () => {
      const r = await post('/api/deadlock');
      $('#deadlock-result').innerHTML = `<b>Cycle detected:</b> ${r.cycle.map(esc).join(' → ')}<br><b>Victim:</b> ${esc(r.victim)} · ${esc(r.resolution)}`;
      await load();
    }],
    ['#crash', runPost('/api/crash', 'Crash simulated.')],
    ['#recover', runPost('/api/recover', 'Recovery complete.')],
    ['#expire', runPost('/api/deadlines', 'Deadline check complete.')],
  ];
  document.addEventListener('click', async event => {
    for (const [selector, handler] of ACTIONS) {
      const el = event.target.closest(selector);
      if (!el) continue;
      try { await handler(el, event); } catch (e) { notify(e.message, true); }
    }
  });

  // ---------- forms: every <form data-route="..."> saves to its API route ----------
  const NUMBER_FIELDS = ['donor_id', 'donor_hospital_id', 'hospital_id', 'organ_id', 'recipient_id', 'from_hospital_id', 'to_hospital_id',
    'urgency_score', 'waiting_days', 'viable_hours', 'distance_km'];
  document.addEventListener('submit', async event => {
    const loginForm = event.target.closest('#login-form');
    if (loginForm) {
      event.preventDefault(); const values=Object.fromEntries(new FormData(loginForm));
      try { const result=await api('/api/login',{method:'POST',body:JSON.stringify(values)}); auth=result; localStorage.setItem('organ-auth',JSON.stringify(auth)); workspace(); notify('Signed in successfully.'); }
      catch(e){notify(e.message,true);} return;
    }
    const hospitalForm = event.target.closest('#hospital-request-form, #hospital-offer-form');
    if (hospitalForm) {
      event.preventDefault(); const values=Object.fromEntries(new FormData(hospitalForm));
      if (hospitalForm.id === 'hospital-request-form') { values.patient_age=Number(values.patient_age); values.urgency_score=Number(values.urgency_score); }
      else { values.available_at=new Date(values.available_at).toISOString(); values.expires_at=new Date(values.expires_at).toISOString(); }
      try { await api(hospitalForm.id==='hospital-request-form'?'/api/hospital/requests':'/api/hospital/offers',{method:'POST',body:JSON.stringify(values)}); hospitalForm.reset(); await load(); notify('Submitted to the admin queue.'); }
      catch(e){notify(e.message,true);} return;
    }
    const processForm = event.target.closest('#process-scheduler-form');
    if (processForm) {
      event.preventDefault();
      const values = Object.fromEntries(new FormData(processForm));
      const jobs = ['P1', 'P2', 'P3', 'P4'].map((id, i) => ({
        id,
        arrival_time: Number(values[`p${i + 1}_arrival`]),
        burst_time: Number(values[`p${i + 1}_burst`]),
        priority: Number(values[`p${i + 1}_priority`]),
      }));
      const button = processForm.querySelector('button[type="submit"]');
      button.disabled = true;
      button.textContent = 'Scheduling…';
      try {
        const result = await api('/api/process-schedule', {
          method: 'POST', body: JSON.stringify({ algorithm: values.algorithm, quantum: Number(values.quantum), jobs }),
        });
        renderProcessSchedule(result);
        notify(`${result.algorithm} scheduling complete.`);
      } catch (e) { notify(e.message, true); }
      finally { button.disabled = false; button.textContent = 'Run scheduler'; }
      return;
    }
    const form = event.target.closest('form[data-route]');
    if (!form) return;
    event.preventDefault();
    const values = Object.fromEntries(new FormData(form));
    for (const key of NUMBER_FIELDS) if (key in values) values[key] = values[key] === '' ? null : Number(values[key]);
    for (const key of ['available_at','expires_at']) if (key in values) values[key] = new Date(values[key]).toISOString();
    if ('medically_eligible' in values) values.medically_eligible = values.medically_eligible === 'true';
    try {
      await api(form.dataset.route, { method: form.dataset.method || 'POST', body: JSON.stringify(values) });
      form.reset();
      await load();
      notify('Saved to database.');
    } catch (e) { notify(e.message, true); }
  });

  // ---------- start ----------
  if (localStorage.getItem(THEME_KEY) === 'dark') document.body.classList.add('dark-mode');
  const startView = location.hash.slice(1);
  if (['records', 'systems'].includes(startView)) showView(startView);
  if (auth?.token) workspace();
  else { auth=null; workspace(); }
  setInterval(() => { if (auth) load(); }, 10000);
})();
