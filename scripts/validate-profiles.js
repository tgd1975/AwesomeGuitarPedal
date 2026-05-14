#!/usr/bin/env node
'use strict';

const fs   = require('fs');
const path = require('path');
const Ajv  = require('ajv');

const ROOT             = path.resolve(__dirname, '..');
const SCHEMA_PATH      = path.join(ROOT, 'data', 'profiles.schema.json');
const PIN_NAMES_PATH   = path.join(ROOT, 'data', 'pin-names.schema.json');
const PROFILES_DIR     = path.join(ROOT, 'profiles');

const schema   = JSON.parse(fs.readFileSync(SCHEMA_PATH, 'utf8'));
const pinNames = JSON.parse(fs.readFileSync(PIN_NAMES_PATH, 'utf8'));
const ajv      = new Ajv({ strict: false });
// EPIC-029: profiles.schema.json $refs "pin-names.schema.json" for the pinRef
// vocabulary. Register the referenced schema *before* compiling profiles so
// AJV resolves the $ref instead of throwing MissingRefError.
ajv.addSchema(pinNames);
const validate = ajv.compile(schema);

function walkDir(dir, results = []) {
  for (const entry of fs.readdirSync(dir, { withFileTypes: true })) {
    const full = path.join(dir, entry.name);
    if (entry.isDirectory()) {
      walkDir(full, results);
    } else if (entry.isFile() && entry.name.endsWith('.json') && entry.name !== 'index.json') {
      results.push(full);
    }
  }
  return results;
}

const files  = walkDir(PROFILES_DIR).sort();
let   errors = 0;

for (const filePath of files) {
  const rel = path.relative(ROOT, filePath);
  let data;
  try {
    data = JSON.parse(fs.readFileSync(filePath, 'utf8'));
  } catch (err) {
    console.error(`PARSE ERROR  ${rel}: ${err.message}`);
    errors++;
    continue;
  }

  const valid = validate(data);
  if (!valid) {
    console.error(`INVALID      ${rel}`);
    for (const e of validate.errors) {
      console.error(`  ${e.instancePath || '(root)'} ${e.message}`);
    }
    errors++;
  } else {
    console.log(`OK           ${rel}`);
  }
}

if (errors > 0) {
  console.error(`\n${errors} file(s) failed validation.`);
  process.exit(1);
} else {
  console.log(`\nAll ${files.length} profile file(s) valid.`);
}
