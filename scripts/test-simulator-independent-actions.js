#!/usr/bin/env node
//
// TASK-307: smoke test for independent-actions support in docs/simulator/simulator.js.
// Loads simulator.js into a sandboxed VM with a minimal DOM stub and verifies:
//   1. A profiles.json containing independentActions loads without error.
//   2. Pressing a button fires both the profile action and the independent action.
//   3. Switching profile leaves the independent action set intact.
//   4. A profiles.json without independentActions is a no-op (only profile fires).
//
// No test framework — kept hand-rolled to avoid adding Jest/Vitest for a single suite.

'use strict';

const fs = require('fs');
const path = require('path');
const vm = require('vm');

const SIM_PATH = path.join(__dirname, '..', 'docs', 'simulator', 'simulator.js');
const FIXTURE_PATH = path.join(__dirname, '..', 'docs', 'simulator', 'test-fixtures', 'independent-actions.json');

let testCount = 0;
let failCount = 0;

function assert(cond, msg) {
  testCount++;
  if (!cond) { failCount++; console.error('  FAIL:', msg); }
  else { console.log('  ok:', msg); }
}

// --- Minimal DOM stub ---------------------------------------------------------
function makeElement(id) {
  const listeners = {};
  const classes = new Set();
  return {
    id,
    _listeners: listeners,
    classList: {
      add: (c) => classes.add(c),
      remove: (c) => classes.delete(c),
      toggle: (c, on) => { if (on) classes.add(c); else classes.delete(c); },
      contains: (c) => classes.has(c),
    },
    style: { setProperty: () => {} },
    innerHTML: '',
    textContent: '',
    value: '',
    files: [],
    dataset: {},
    appendChild: () => {},
    removeChild: () => {},
    querySelector: () => makeElement('child'),
    addEventListener: (ev, fn) => { listeners[ev] = fn; },
    click: () => {},
  };
}

function makeDom() {
  const elements = {};
  const get = (id) => {
    if (!elements[id]) elements[id] = makeElement(id);
    return elements[id];
  };
  const document = {
    _elements: elements,
    getElementById: get,
    createElement: () => makeElement('created'),
    addEventListener: (ev, fn) => { if (ev === 'DOMContentLoaded') queueMicrotask(fn); },
    body: { appendChild: () => {}, removeChild: () => {} },
  };
  return document;
}

// --- Load simulator into a sandbox ---------------------------------------------
function loadSimulator() {
  const document = makeDom();
  const window = { addEventListener: () => {} };
  const sandbox = {
    document,
    window,
    setTimeout, clearTimeout, setInterval, clearInterval,
    queueMicrotask,
    Date,
    Math,
    Object, Array, JSON, Number, String, Boolean, Set, Map, Symbol, Error,
    URL: { createObjectURL: () => 'blob:', revokeObjectURL: () => {} },
    Blob: function() {},
    FileReader: function() {},
    console,
    ProfilesGallery: function() { return { open: () => {} }; },
  };
  vm.createContext(sandbox);
  const code = fs.readFileSync(SIM_PATH, 'utf8');
  vm.runInContext(code, sandbox, { filename: 'simulator.js' });
  return { sandbox, document };
}

// --- Test 1: parse with block present, both actions fire on press --------------
function test_bothActionsFire() {
  console.log('\nTest 1: independent + profile actions both fire on press');
  const { sandbox, document } = loadSimulator();

  const bleLines = [];
  const serialLines = [];
  document._elements['ble-output'] = makeElement('ble-output');
  document._elements['serial-output'] = makeElement('serial-output');
  document._elements['ble-output'].appendChild = (line) => bleLines.push(line.textContent);
  document._elements['serial-output'].appendChild = (line) => serialLines.push(line.textContent);

  const fixture = JSON.parse(fs.readFileSync(FIXTURE_PATH, 'utf8'));
  let loadError = null;
  try { sandbox.loadConfig(fixture); } catch (e) { loadError = e; }
  assert(loadError === null, 'fixture loads without throwing');
  assert(sandbox.independentActionFor('A') !== null, 'independent action for A is registered');
  assert(sandbox.independentActionFor('B') !== null, 'independent action for B is registered');

  // Press slot A — profile action is SendCharAction (BLE log), independent is SerialOutputAction.
  sandbox.handleButtonDown('A');
  sandbox.handleButtonUp('A');

  const indFired = serialLines.some(l => l.includes('Button A pressed (independent)'));
  const profileFired = bleLines.some(l => l.includes('KEY_PAGE_UP'));
  assert(indFired, 'independent action (serial) fired on press of A');
  assert(profileFired, 'profile action (BLE keystroke) fired on press of A');

  // Press slot B — no profile binding, only independent should fire.
  serialLines.length = 0;
  bleLines.length = 0;
  sandbox.handleButtonDown('B');
  sandbox.handleButtonUp('B');
  const bIndFired = serialLines.some(l => l.includes('Button B has no profile binding'));
  assert(bIndFired, 'independent action fires on B even with no profile entry');
  assert(bleLines.length === 0, 'no BLE traffic for B (no profile action exists)');
}

// --- Test 2: profile switch leaves independents intact -------------------------
function test_profileSwitchLeavesIndependentsIntact() {
  console.log('\nTest 2: switching profile preserves independent actions');
  const { sandbox, document } = loadSimulator();

  const bleLines = [];
  const serialLines = [];
  document._elements['ble-output'] = makeElement('ble-output');
  document._elements['serial-output'] = makeElement('serial-output');
  document._elements['ble-output'].appendChild = (line) => bleLines.push(line.textContent);
  document._elements['serial-output'].appendChild = (line) => serialLines.push(line.textContent);

  const fixture = JSON.parse(fs.readFileSync(FIXTURE_PATH, 'utf8'));
  sandbox.loadConfig(fixture);

  // On profile 0: profile action is KEY_PAGE_UP, independent is serial.
  sandbox.handleButtonDown('A');
  sandbox.handleButtonUp('A');
  const profile0Fired = bleLines.some(l => l.includes('KEY_PAGE_UP'));
  const indFiredOn0 = serialLines.some(l => l.includes('Button A pressed (independent)'));
  assert(profile0Fired, 'profile 0 action (KEY_PAGE_UP) fires before switch');
  assert(indFiredOn0, 'independent fires on profile 0');

  // Switch to profile 1.
  bleLines.length = 0;
  serialLines.length = 0;
  sandbox.selectProfile(1);
  assert(sandbox.independentActionFor('A') !== null, 'independent for A still registered after profile switch');

  // Press A on profile 1: profile action should now be MEDIA_PLAY_PAUSE, independent unchanged.
  sandbox.handleButtonDown('A');
  sandbox.handleButtonUp('A');
  const profile1Fired = bleLines.some(l => l.includes('MEDIA_PLAY_PAUSE'));
  const indFiredOn1 = serialLines.some(l => l.includes('Button A pressed (independent)'));
  assert(profile1Fired, 'profile 1 action (MEDIA_PLAY_PAUSE) fires after switch');
  assert(indFiredOn1, 'independent action still fires after switching profile');
}

// --- Test 3: missing block is a no-op ------------------------------------------
function test_missingBlockIsNoop() {
  console.log('\nTest 3: profiles.json without independentActions behaves identically');
  const { sandbox, document } = loadSimulator();

  const bleLines = [];
  const serialLines = [];
  document._elements['ble-output'] = makeElement('ble-output');
  document._elements['serial-output'] = makeElement('serial-output');
  document._elements['ble-output'].appendChild = (line) => bleLines.push(line.textContent);
  document._elements['serial-output'].appendChild = (line) => serialLines.push(line.textContent);

  const noBlock = {
    profiles: [{
      name: 'P1',
      buttons: { A: { type: 'SendCharAction', name: 'Up', value: 'KEY_UP' } },
    }],
  };
  let loadError = null;
  try { sandbox.loadConfig(noBlock); } catch (e) { loadError = e; }
  assert(loadError === null, 'config without independentActions loads without throwing');
  assert(sandbox.independentActionFor('A') === null, 'no independent action returned for A');

  sandbox.handleButtonDown('A');
  sandbox.handleButtonUp('A');
  assert(bleLines.some(l => l.includes('KEY_UP')), 'profile action still fires');
  assert(serialLines.length === 0, 'no spurious serial output (no independents present)');
}

// --- Run ----------------------------------------------------------------------
test_bothActionsFire();
test_profileSwitchLeavesIndependentsIntact();
test_missingBlockIsNoop();

console.log(`\n${testCount - failCount}/${testCount} assertions passed.`);
process.exit(failCount === 0 ? 0 : 1);
