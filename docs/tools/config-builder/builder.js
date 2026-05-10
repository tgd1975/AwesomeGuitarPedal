'use strict';

const ACTION_TYPES = [
  { label: '(none)', value: '' },
  { label: 'Key (named)', value: 'SendCharAction' },
  { label: 'Key (raw HID)', value: 'SendKeyAction' },
  { label: 'Type String', value: 'SendStringAction' },
  { label: 'Media Key', value: 'SendMediaKeyAction' },
  { label: 'Serial Output', value: 'SerialOutputAction' },
  { label: 'Delayed Action', value: 'DelayedAction' },
  { label: 'Pin High', value: 'PinHighAction' },
  { label: 'Pin Low', value: 'PinLowAction' },
  { label: 'Pin Toggle', value: 'PinToggleAction' },
  { label: 'Pin High While Pressed', value: 'PinHighWhilePressedAction' },
  { label: 'Pin Low While Pressed', value: 'PinLowWhilePressedAction' },
];

const MEDIA_KEY_VALUES = [
  'MEDIA_NEXT_TRACK', 'KEY_MEDIA_NEXT_TRACK',
  'MEDIA_PREVIOUS_TRACK', 'KEY_MEDIA_PREVIOUS_TRACK',
  'MEDIA_STOP', 'KEY_MEDIA_STOP',
  'MEDIA_PLAY_PAUSE', 'KEY_MEDIA_PLAY_PAUSE',
  'MEDIA_MUTE', 'KEY_MEDIA_MUTE',
  'MEDIA_VOLUME_UP', 'KEY_MEDIA_VOLUME_UP', 'KEY_VOLUME_UP',
  'MEDIA_VOLUME_DOWN', 'KEY_MEDIA_VOLUME_DOWN', 'KEY_VOLUME_DOWN',
  'KEY_MEDIA_WWW_HOME', 'KEY_MEDIA_WWW_BACK', 'KEY_MEDIA_WWW_STOP',
  'KEY_MEDIA_WWW_SEARCH', 'KEY_MEDIA_WWW_BOOKMARKS',
  'KEY_MEDIA_CALCULATOR', 'KEY_MEDIA_EMAIL_READER',
  'KEY_MEDIA_LOCAL_MACHINE_BROWSER', 'KEY_MEDIA_CONSUMER_CONTROL_CONFIGURATION',
];

const KEY_NAMES = [
  'KEY_LEFT_ARROW', 'LEFT_ARROW', 'KEY_RIGHT_ARROW', 'RIGHT_ARROW',
  'KEY_UP_ARROW', 'UP_ARROW', 'KEY_DOWN_ARROW', 'DOWN_ARROW',
  'KEY_PAGE_UP', 'KEY_PAGE_DOWN', 'KEY_HOME', 'KEY_END',
  'KEY_INSERT', 'KEY_DELETE',
  'KEY_BACKSPACE', 'KEY_TAB', 'KEY_RETURN', 'KEY_ENTER',
  'KEY_ESC', 'KEY_CAPS_LOCK', 'KEY_PRINTSCREEN',
  'KEY_LEFT_CTRL', 'KEY_LEFT_SHIFT', 'KEY_LEFT_ALT', 'KEY_LEFT_GUI',
  'KEY_RIGHT_CTRL', 'KEY_RIGHT_SHIFT', 'KEY_RIGHT_ALT', 'KEY_RIGHT_GUI',
  ...Array.from({ length: 24 }, (_, i) => `KEY_F${i + 1}`),
  ...Array.from({ length: 24 }, (_, i) => `F${i + 1}`),
  'KEY_NUM_0', 'KEY_NUM_1', 'KEY_NUM_2', 'KEY_NUM_3', 'KEY_NUM_4',
  'KEY_NUM_5', 'KEY_NUM_6', 'KEY_NUM_7', 'KEY_NUM_8', 'KEY_NUM_9',
  'KEY_NUM_SLASH', 'KEY_NUM_ASTERISK', 'KEY_NUM_MINUS',
  'KEY_NUM_PLUS', 'KEY_NUM_ENTER', 'KEY_NUM_PERIOD',
];

const PIN_TYPES = [
  'PinHighAction', 'PinLowAction', 'PinToggleAction',
  'PinHighWhilePressedAction', 'PinLowWhilePressedAction',
];

const BUTTON_SLOTS = ['A', 'B', 'C', 'D'];

// Built-in example profile — demonstrates the four most common action types.
const EXAMPLE_PROFILES_JSON = {
  _doc: 'Key reference: docs/builders/KEY_REFERENCE.md',
  profiles: [
    {
      name: 'Example Profile',
      description: 'Demonstrates common action types. Replace with your own mappings.',
      buttons: {
        A: { type: 'SendKeyAction',      name: 'Page Down', value: 'KEY_PAGE_DOWN' },
        B: { type: 'SendMediaKeyAction', name: 'Play/Pause', value: 'MEDIA_PLAY_PAUSE' },
        C: { type: 'SerialOutputAction', name: 'Debug Ping', value: 'AwesomeStudioPedal: ping' },
        D: { type: 'SendStringAction',   name: 'Type Text',  value: 'Hello from pedal!' },
      },
    },
  ],
};

let profiles = [];
let activeIndex = 0;
let validateFn = null;

// Independent actions — top-level block in profiles.json that fires on every
// button event regardless of which profile is active. Stored as a slot-keyed
// object of action editor states. Empty slots are not serialised; if every
// slot is empty the whole block is omitted from the output JSON.
let independentActions = Object.fromEntries(BUTTON_SLOTS.map(s => [s, emptyAction()]));

function emptyAction() {
  return { type: '', name: '', value: '', delayMs: 1000, pin: 0, action: null, longPress: null, doublePress: null };
}

function emptyProfile() {
  return {
    name: 'New Profile',
    description: '',
    buttons: Object.fromEntries(BUTTON_SLOTS.map(s => [s, emptyAction()])),
  };
}

function esc(s) {
  return String(s)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&#x27;');
}

async function init() {
  try {
    const res = await fetch('./schema.json');
    const schema = await res.json();
    const AjvClass = window.Ajv2019 || window.Ajv || (window.ajv && window.ajv.default);
    if (AjvClass) {
      const ajvInstance = new AjvClass({ allErrors: true, strict: false });
      validateFn = ajvInstance.compile(schema);
    }
  } catch (e) {
    console.warn('Schema/validator init failed:', e);
  }

  // Auto-load example on startup (TASK-092)
  populateForm(EXAMPLE_PROFILES_JSON);
  showExampleNotice(true);
}

function showExampleNotice(visible) {
  let el = document.getElementById('example-notice');
  if (!el) return;
  el.style.display = visible ? 'block' : 'none';
}

function renderAll() {
  renderTabs();
  renderProfileForm();
  renderIndependentActions();
  updatePreview();
}

function renderTabs() {
  const el = document.getElementById('profiles-tabs');
  el.innerHTML = '';

  profiles.forEach((p, i) => {
    const btn = document.createElement('button');
    btn.className = 'tab-btn' + (i === activeIndex ? ' active' : '');
    btn.textContent = (p.name || `Profile ${i + 1}`).substring(0, 22);
    btn.onclick = () => { activeIndex = i; renderAll(); };
    el.appendChild(btn);
  });

  if (profiles.length < 7) {
    const addBtn = document.createElement('button');
    addBtn.className = 'tab-btn add-btn';
    addBtn.textContent = '+ Add Profile';
    addBtn.onclick = addProfile;
    el.appendChild(addBtn);
  }
}

function makeSubActionDetails(label, getAct, onUpdate) {
  const details = document.createElement('details');
  details.className = 'sub-action-details';

  const current = getAct();
  if (current && current.type) details.open = true;

  const summary = document.createElement('summary');
  summary.className = 'sub-action-summary';
  summary.textContent = label;
  details.appendChild(summary);

  const inner = document.createElement('div');
  inner.className = 'sub-action-inner';

  renderActionFields(
    inner,
    () => getAct() || emptyAction(),
    (v) => {
      // Treat setting type to '' as clearing the sub-action
      onUpdate(v.type ? v : null);
      // Collapse if cleared
      if (!v.type) details.open = false;
    },
    true  // nested = true; prevents DelayedAction from appearing
  );

  details.appendChild(inner);
  return details;
}

function renderProfileForm() {
  const p = profiles[activeIndex];
  const form = document.getElementById('active-profile-form');
  form.innerHTML = '';

  const nameInput = document.createElement('input');
  nameInput.type = 'text';
  nameInput.value = p.name;
  nameInput.placeholder = 'Profile name';
  nameInput.oninput = () => { p.name = nameInput.value; renderTabs(); updatePreview(); };
  form.appendChild(makeField('Profile Name', nameInput));

  const descInput = document.createElement('textarea');
  descInput.rows = 2;
  descInput.value = p.description;
  descInput.placeholder = 'Optional description';
  descInput.oninput = () => { p.description = descInput.value; updatePreview(); };
  form.appendChild(makeField('Description (optional)', descInput));

  const grid = document.createElement('div');
  grid.className = 'buttons-grid';

  for (const slot of BUTTON_SLOTS) {
    const row = document.createElement('div');
    row.className = 'button-row';

    const lbl = document.createElement('span');
    lbl.className = 'button-label';
    lbl.textContent = `Button ${slot}`;

    const actionDiv = document.createElement('div');
    actionDiv.className = 'action-container';

    renderActionFields(
      actionDiv,
      () => p.buttons[slot],
      (v) => { p.buttons[slot] = v; updatePreview(); }
    );

    // Long Press sub-action
    actionDiv.appendChild(makeSubActionDetails(
      '\u23f3 Long Press',
      () => p.buttons[slot].longPress,
      (v) => { p.buttons[slot] = { ...p.buttons[slot], longPress: v }; updatePreview(); }
    ));

    // Double Press sub-action
    actionDiv.appendChild(makeSubActionDetails(
      '2\u00d7 Double Press',
      () => p.buttons[slot].doublePress,
      (v) => { p.buttons[slot] = { ...p.buttons[slot], doublePress: v }; updatePreview(); }
    ));

    row.appendChild(lbl);
    row.appendChild(actionDiv);
    grid.appendChild(row);
  }

  form.appendChild(grid);

  if (profiles.length > 1) {
    const removeBtn = document.createElement('button');
    removeBtn.className = 'btn-danger';
    removeBtn.textContent = `Remove Profile "${p.name || `Profile ${activeIndex + 1}`}"`;
    removeBtn.onclick = () => removeProfile(activeIndex);
    form.appendChild(removeBtn);
  }
}

function renderIndependentActions() {
  const host = document.getElementById('independent-actions-section');
  if (!host) return;
  host.innerHTML = '';

  const details = document.createElement('details');
  details.className = 'independent-actions';
  if (hasAnyIndependentAction()) details.open = true;

  const summary = document.createElement('summary');
  summary.className = 'independent-actions-summary';
  summary.innerHTML =
    '★ Independent actions ' +
    '<span class="ind-help">(fire on every button event regardless of profile)</span>';
  details.appendChild(summary);

  const grid = document.createElement('div');
  grid.className = 'buttons-grid';

  for (const slot of BUTTON_SLOTS) {
    const row = document.createElement('div');
    row.className = 'button-row';

    const lbl = document.createElement('span');
    lbl.className = 'button-label';
    lbl.textContent = `Button ${slot}`;

    const actionDiv = document.createElement('div');
    actionDiv.className = 'action-container';

    renderActionFields(
      actionDiv,
      () => independentActions[slot],
      (v) => { independentActions[slot] = v; updatePreview(); }
    );

    actionDiv.appendChild(makeSubActionDetails(
      '⏳ Long Press',
      () => independentActions[slot].longPress,
      (v) => { independentActions[slot] = { ...independentActions[slot], longPress: v }; updatePreview(); }
    ));

    actionDiv.appendChild(makeSubActionDetails(
      '2× Double Press',
      () => independentActions[slot].doublePress,
      (v) => { independentActions[slot] = { ...independentActions[slot], doublePress: v }; updatePreview(); }
    ));

    row.appendChild(lbl);
    row.appendChild(actionDiv);
    grid.appendChild(row);
  }

  details.appendChild(grid);

  if (hasAnyIndependentAction()) {
    const clearBtn = document.createElement('button');
    clearBtn.className = 'btn-danger';
    clearBtn.textContent = 'Remove all independent actions';
    clearBtn.onclick = () => {
      independentActions = Object.fromEntries(BUTTON_SLOTS.map(s => [s, emptyAction()]));
      renderAll();
    };
    details.appendChild(clearBtn);
  }

  host.appendChild(details);
}

function hasAnyIndependentAction() {
  for (const slot of BUTTON_SLOTS) {
    const a = independentActions[slot];
    if (a && a.type) return true;
  }
  return false;
}

function makeField(labelText, input) {
  const row = document.createElement('div');
  row.className = 'field-row';
  const lbl = document.createElement('label');
  lbl.textContent = labelText;
  row.appendChild(lbl);
  row.appendChild(input);
  return row;
}

function renderActionFields(container, getAct, onUpdate, nested = false) {
  container.innerHTML = '';

  const typeSelect = document.createElement('select');
  typeSelect.className = 'action-type-select';
  for (const opt of ACTION_TYPES) {
    if (nested && opt.value === 'DelayedAction') continue;
    const o = document.createElement('option');
    o.value = opt.value;
    o.textContent = opt.label;
    o.selected = getAct().type === opt.value;
    typeSelect.appendChild(o);
  }
  container.appendChild(typeSelect);

  const fieldsDiv = document.createElement('div');
  fieldsDiv.className = 'action-fields';
  container.appendChild(fieldsDiv);

  function refreshFields() {
    const a = getAct();
    fieldsDiv.innerHTML = '';
    if (!a.type) return;

    const nameInput = document.createElement('input');
    nameInput.type = 'text';
    nameInput.value = a.name || '';
    nameInput.placeholder = 'Button label (optional)';
    nameInput.oninput = () => onUpdate({ ...getAct(), name: nameInput.value });
    fieldsDiv.appendChild(makeField('Name', nameInput));

    if (a.type === 'SendCharAction' || a.type === 'SendKeyAction') {
      const valInput = document.createElement('input');
      valInput.type = 'text';
      valInput.value = a.value || '';
      valInput.setAttribute('list', 'key-names-list');
      valInput.placeholder = 'e.g. KEY_PAGE_DOWN or a';
      valInput.oninput = () => onUpdate({ ...getAct(), value: valInput.value });
      fieldsDiv.appendChild(makeField('Key value', valInput));

    } else if (a.type === 'SendStringAction' || a.type === 'SerialOutputAction') {
      const valTA = document.createElement('textarea');
      valTA.rows = 2;
      valTA.value = a.value || '';
      valTA.placeholder = a.type === 'SerialOutputAction' ? 'Debug message' : 'Text to type';
      valTA.oninput = () => onUpdate({ ...getAct(), value: valTA.value });
      fieldsDiv.appendChild(makeField('Value', valTA));

    } else if (a.type === 'SendMediaKeyAction') {
      const valSel = document.createElement('select');
      for (const v of MEDIA_KEY_VALUES) {
        const o = document.createElement('option');
        o.value = v;
        o.textContent = v;
        o.selected = a.value === v;
        valSel.appendChild(o);
      }
      if (!a.value) onUpdate({ ...getAct(), value: MEDIA_KEY_VALUES[0] });
      valSel.onchange = () => onUpdate({ ...getAct(), value: valSel.value });
      fieldsDiv.appendChild(makeField('Media key', valSel));

    } else if (a.type === 'DelayedAction') {
      const delayInput = document.createElement('input');
      delayInput.type = 'number';
      delayInput.min = 0;
      delayInput.value = a.delayMs ?? 1000;
      delayInput.oninput = () => onUpdate({ ...getAct(), delayMs: parseInt(delayInput.value) || 0 });
      fieldsDiv.appendChild(makeField('Delay (ms)', delayInput));

      const thenLabel = document.createElement('div');
      thenLabel.className = 'nested-label';
      thenLabel.textContent = 'Then execute:';
      fieldsDiv.appendChild(thenLabel);

      const nestedDiv = document.createElement('div');
      nestedDiv.className = 'nested-action-container';
      renderActionFields(
        nestedDiv,
        () => getAct().action || emptyAction(),
        (newInner) => onUpdate({ ...getAct(), action: newInner }),
        true
      );
      fieldsDiv.appendChild(nestedDiv);

    } else if (PIN_TYPES.includes(a.type)) {
      const pinInput = document.createElement('input');
      pinInput.type = 'number';
      pinInput.min = 0;
      pinInput.max = 39;
      pinInput.value = a.pin ?? 0;
      pinInput.oninput = () => onUpdate({ ...getAct(), pin: parseInt(pinInput.value) || 0 });
      fieldsDiv.appendChild(makeField('GPIO pin (0–39)', pinInput));
    }
  }

  typeSelect.onchange = () => {
    const current = getAct();
    onUpdate({ ...emptyAction(), type: typeSelect.value, name: current.name });
    refreshFields();
  };

  refreshFields();
}

function buildJson() {
  const out = {
    _doc: 'Key reference: docs/builders/KEY_REFERENCE.md',
    profiles: profiles.map(profileToJson),
  };
  // Independent actions: omit key entirely when no slot is set, never write {}.
  const ind = independentActionsToJson();
  if (ind) out.independentActions = ind;
  return out;
}

function independentActionsToJson() {
  const r = {};
  for (const slot of BUTTON_SLOTS) {
    const a = independentActions[slot];
    if (a && a.type) r[slot] = actionToJson(a);
  }
  return Object.keys(r).length === 0 ? null : r;
}

function profileToJson(p) {
  const r = { name: p.name };
  if (p.description) r.description = p.description;
  r.buttons = {};
  for (const slot of BUTTON_SLOTS) {
    const a = p.buttons[slot];
    if (a && a.type) r.buttons[slot] = actionToJson(a);
  }
  return r;
}

function actionToJson(a) {
  const r = { type: a.type };
  if (a.name) r.name = a.name;
  if (a.type === 'DelayedAction') {
    r.delayMs = parseInt(a.delayMs) || 0;
    if (a.action && a.action.type) r.action = actionToJson(a.action);
  } else if (PIN_TYPES.includes(a.type)) {
    r.pin = parseInt(a.pin) || 0;
  } else {
    r.value = a.value || '';
  }
  if (a.longPress && a.longPress.type) r.longPress = actionToJson(a.longPress);
  if (a.doublePress && a.doublePress.type) r.doublePress = actionToJson(a.doublePress);
  return r;
}

function validateJson(json) {
  if (!validateFn) return { valid: null, errors: ['Schema validator not available (check CDN connectivity)'] };
  const valid = validateFn(json);
  if (valid) return { valid: true, errors: [] };
  return {
    valid: false,
    errors: (validateFn.errors || []).map(e => {
      const path = e.instancePath || '';
      return path ? `${path}: ${e.message}` : e.message;
    }),
  };
}

function downloadJson() {
  const json = buildJson();
  const blob = new Blob([JSON.stringify(json, null, 2)], { type: 'application/json' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = 'profiles.json';
  document.body.appendChild(a);
  a.click();
  document.body.removeChild(a);
  URL.revokeObjectURL(url);
}

function loadFromFile(file) {
  const reader = new FileReader();
  reader.onload = (e) => {
    try {
      const json = JSON.parse(e.target.result);
      populateForm(json);
    } catch (err) {
      showValidationResult(false, [`Parse error: ${err.message}`]);
    }
  };
  reader.readAsText(file);
}

function populateForm(json) {
  if (!json.profiles || !Array.isArray(json.profiles)) {
    showValidationResult(false, ['Missing or invalid "profiles" array']);
    return;
  }
  profiles = json.profiles.map(p => ({
    name: p.name || '',
    description: p.description || '',
    buttons: Object.fromEntries(
      BUTTON_SLOTS.map(slot => [
        slot,
        p.buttons && p.buttons[slot] ? actionFromJson(p.buttons[slot]) : emptyAction(),
      ])
    ),
  }));
  independentActions = Object.fromEntries(
    BUTTON_SLOTS.map(slot => [
      slot,
      json.independentActions && json.independentActions[slot]
        ? actionFromJson(json.independentActions[slot])
        : emptyAction(),
    ])
  );
  activeIndex = 0;
  renderAll();
  showValidationResult(true, []);
  showExampleNotice(false);
}

function actionFromJson(a) {
  const r = emptyAction();
  r.type = a.type || '';
  r.name = a.name || '';
  if (a.type === 'DelayedAction') {
    r.delayMs = a.delayMs ?? 1000;
    r.action = a.action ? actionFromJson(a.action) : emptyAction();
  } else if (PIN_TYPES.includes(a.type)) {
    r.pin = a.pin ?? 0;
  } else {
    r.value = a.value || '';
  }
  r.longPress = a.longPress ? actionFromJson(a.longPress) : null;
  r.doublePress = a.doublePress ? actionFromJson(a.doublePress) : null;
  return r;
}

function addProfile() {
  if (profiles.length >= 7) return;
  profiles.push(emptyProfile());
  activeIndex = profiles.length - 1;
  renderAll();
}

function removeProfile(index) {
  profiles.splice(index, 1);
  activeIndex = Math.min(activeIndex, profiles.length - 1);
  renderAll();
}

function updatePreview() {
  document.getElementById('json-preview').textContent = JSON.stringify(buildJson(), null, 2);
}

function showValidationResult(valid, errors) {
  const el = document.getElementById('validation-result');
  if (valid === true) {
    el.className = 'validation-ok';
    el.textContent = '\u2713 Valid \u2014 ready to download';
  } else if (valid === false) {
    el.className = 'validation-error';
    el.innerHTML =
      '<strong>\u2717 Validation errors:</strong><ul>' +
      errors.map(e => `<li>${esc(e)}</li>`).join('') +
      '</ul>';
  } else {
    el.className = 'validation-warn';
    el.textContent = errors.join('; ');
  }
}

document.addEventListener('DOMContentLoaded', () => {
  document.getElementById('btn-validate').onclick = () => {
    const result = validateJson(buildJson());
    showValidationResult(result.valid, result.errors);
  };

  document.getElementById('btn-download').onclick = downloadJson;

  document.getElementById('btn-load').onclick = () =>
    document.getElementById('file-input').click();

  document.getElementById('file-input').onchange = (e) => {
    if (e.target.files[0]) {
      loadFromFile(e.target.files[0]);
      e.target.value = '';
    }
  };

  document.getElementById('btn-community').onclick = () => {
    const gallery = new ProfilesGallery({
      indexUrl:    'https://tgd1975.github.io/AwesomeStudioPedal/profiles/index.json',
      baseUrl:     'https://tgd1975.github.io/AwesomeStudioPedal/profiles/',
      onLoad:      (json) => populateForm(json),
      buttonCount: BUTTON_SLOTS.length || null,
    });
    gallery.open();
  };

  init();
});

// Test-only hook: exposes the closure-scoped state setters/getters so the
// hand-rolled VM test in scripts/test-config-builder-independent-actions.js
// can verify state without a real DOM. Browsers ignore this.
if (typeof globalThis !== 'undefined') {
  globalThis.__cb_test = {
    getIndependentActions: () => independentActions,
    setIndependentActionSlot: (slot, action) => { independentActions[slot] = action; },
    resetIndependentActions: () => {
      independentActions = Object.fromEntries(BUTTON_SLOTS.map(s => [s, emptyAction()]));
    },
    hasAnyIndependentAction,
    independentActionsToJson,
  };
}
