#!/usr/bin/env node
//
// TASK-308: smoke test for independent-actions support in
// docs/tools/config-builder/builder.js. Loads builder.js into a VM with a
// minimal DOM stub and verifies the data round-trip:
//
//   1. Loading a file with independentActions populates state.
//   2. Re-serialising round-trips the block exactly.
//   3. Adding an entry produces JSON with the block.
//   4. Clearing all entries omits the key (does not write {}).
//   5. Loading a file without the block leaves state empty.
//
// No new test framework — kept hand-rolled to match TASK-307 (simulator).

'use strict';

const fs = require('fs');
const path = require('path');
const vm = require('vm');

const BUILDER_PATH = path.join(__dirname, '..', 'docs', 'tools', 'config-builder', 'builder.js');

let testCount = 0;
let failCount = 0;
function assert(cond, msg) {
  testCount++;
  if (!cond) { failCount++; console.error('  FAIL:', msg); }
  else { console.log('  ok:', msg); }
}

function makeElement(id) {
  const classes = new Set();
  return {
    id,
    classList: {
      add: (c) => classes.add(c),
      remove: (c) => classes.delete(c),
      toggle: (c, on) => { if (on) classes.add(c); else classes.delete(c); },
      contains: (c) => classes.has(c),
    },
    style: {},
    innerHTML: '',
    textContent: '',
    value: '',
    files: [],
    dataset: {},
    children: [],
    appendChild: function(c) { this.children.push(c); },
    removeChild: () => {},
    querySelector: () => makeElement('child'),
    addEventListener: () => {},
    setAttribute: () => {},
    click: () => {},
  };
}

function makeDom() {
  const elements = {};
  const get = (id) => {
    if (!elements[id]) elements[id] = makeElement(id);
    return elements[id];
  };
  return {
    getElementById: get,
    createElement: () => makeElement('created'),
    addEventListener: () => {},   // swallow DOMContentLoaded
    body: { appendChild: () => {}, removeChild: () => {} },
  };
}

function loadBuilder() {
  const document = makeDom();
  const sandbox = {
    document,
    window: { Ajv2019: null, Ajv: null, ajv: null },
    setTimeout, clearTimeout, setInterval, clearInterval,
    queueMicrotask,
    Date, Math,
    Object, Array, JSON, Number, String, Boolean, Set, Map, Symbol, Error,
    URL: { createObjectURL: () => 'blob:', revokeObjectURL: () => {} },
    Blob: function() {},
    FileReader: function() {},
    fetch: () => Promise.reject(new Error('no fetch in test')),
    console,
    ProfilesGallery: function() { return { open: () => {} }; },
  };
  vm.createContext(sandbox);
  const code = fs.readFileSync(BUILDER_PATH, 'utf8');
  vm.runInContext(code, sandbox, { filename: 'builder.js' });
  return sandbox;
}

// --- Test 1: round-trip a file containing independentActions ------------------
function test_roundTripWithBlock() {
  console.log('\nTest 1: round-trip a file containing independentActions');
  const sb = loadBuilder();

  const input = {
    profiles: [{
      name: 'P1',
      buttons: { A: { type: 'SendCharAction', value: 'KEY_PAGE_UP' } },
    }],
    independentActions: {
      A: { type: 'SerialOutputAction', name: 'Log', value: 'press' },
      C: { type: 'PinHighWhilePressedAction', pin: 12 },
    },
  };

  let err = null;
  try { sb.populateForm(input); } catch (e) { err = e; }
  assert(err === null, 'populateForm accepts file with independentActions');

  const out = sb.buildJson();
  assert(out.independentActions, 'output has independentActions');
  assert(out.independentActions.A.type === 'SerialOutputAction', 'A type round-trips');
  assert(out.independentActions.A.value === 'press', 'A value round-trips');
  assert(out.independentActions.A.name === 'Log', 'A name round-trips');
  assert(out.independentActions.C.type === 'PinHighWhilePressedAction', 'C type round-trips');
  assert(out.independentActions.C.pin === 12, 'C pin round-trips');
  assert(out.independentActions.B === undefined, 'B not present (was empty in input)');
  assert(out.independentActions.D === undefined, 'D not present (was empty in input)');
}

// --- Test 2: removing all entries omits the key entirely ----------------------
function test_removeAllOmitsKey() {
  console.log('\nTest 2: removing all entries omits the key (not {})');
  const sb = loadBuilder();

  // Start with a block, then clear every slot.
  sb.populateForm({
    profiles: [{ name: 'P1', buttons: { A: { type: 'SendCharAction', value: 'X' } } }],
    independentActions: { A: { type: 'SerialOutputAction', value: 'foo' } },
  });
  let out = sb.buildJson();
  assert(out.independentActions, 'block present after load');

  // Now clear by re-populating with no independents.
  sb.populateForm({
    profiles: [{ name: 'P1', buttons: { A: { type: 'SendCharAction', value: 'X' } } }],
  });
  out = sb.buildJson();
  assert(!('independentActions' in out), 'independentActions key is absent (not {})');
}

// --- Test 3: loading without the block leaves an empty internal state ---------
function test_loadWithoutBlock() {
  console.log('\nTest 3: loading without independentActions leaves output without it');
  const sb = loadBuilder();

  sb.populateForm({
    profiles: [{ name: 'P1', buttons: { A: { type: 'SendCharAction', value: 'X' } } }],
  });
  const out = sb.buildJson();
  assert(!('independentActions' in out), 'output omits independentActions');
  assert(sb.__cb_test.hasAnyIndependentAction() === false, 'hasAnyIndependentAction is false');
}

// --- Test 4: adding an entry produces JSON with the block ---------------------
function test_addEntry() {
  console.log('\nTest 4: adding an entry to a file without the block produces JSON with it');
  const sb = loadBuilder();

  // Start with no block.
  sb.populateForm({
    profiles: [{ name: 'P1', buttons: { A: { type: 'SendCharAction', value: 'X' } } }],
  });
  let out = sb.buildJson();
  assert(!('independentActions' in out), 'no block initially');

  // Simulate user adding an entry via the UI by reaching through the test hook.
  sb.__cb_test.setIndependentActionSlot('B', {
    type: 'SerialOutputAction', name: 'Added', value: 'hello', delayMs: 1000,
    pin: 0, action: null, longPress: null, doublePress: null,
  });
  out = sb.buildJson();
  assert(out.independentActions, 'block now present after edit');
  assert(out.independentActions.B.type === 'SerialOutputAction', 'added entry serialises');
  assert(out.independentActions.B.value === 'hello', 'added value serialises');
  assert(out.independentActions.A === undefined, 'untouched slot A not in output');

  // After clearing the slot back to empty, block should be omitted again.
  sb.__cb_test.setIndependentActionSlot('B', {
    type: '', name: '', value: '', delayMs: 1000,
    pin: 0, action: null, longPress: null, doublePress: null,
  });
  out = sb.buildJson();
  assert(!('independentActions' in out), 'block removed after clearing the only entry');
}

// --- Test 5: longPress/doublePress on independents round-trip -----------------
function test_longPressOnIndependents() {
  console.log('\nTest 5: longPress on an independent action round-trips');
  const sb = loadBuilder();

  const input = {
    profiles: [{ name: 'P1', buttons: {} }],
    independentActions: {
      A: {
        type: 'SerialOutputAction', value: 'short',
        longPress: { type: 'SerialOutputAction', value: 'long' },
      },
    },
  };
  sb.populateForm(input);
  const out = sb.buildJson();
  assert(out.independentActions.A.longPress, 'longPress preserved');
  assert(out.independentActions.A.longPress.value === 'long', 'longPress value preserved');
}

test_roundTripWithBlock();
test_removeAllOmitsKey();
test_loadWithoutBlock();
test_addEntry();
test_longPressOnIndependents();

console.log(`\n${testCount - failCount}/${testCount} assertions passed.`);
process.exit(failCount === 0 ? 0 : 1);
