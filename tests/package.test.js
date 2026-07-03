'use strict';

const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');

const binding = require('..');

for (const [name, file] of Object.entries(binding)) {
  assert.equal(path.isAbsolute(file), true, `${name} must be an absolute path`);
  assert.equal(fs.existsSync(file), true, `${name} does not exist: ${file}`);
}

assert.equal(
  require.resolve('@obinexusltd/asm-polycall/src/asm_polycall.S'),
  binding.assembly
);
assert.equal(
  require.resolve('@obinexusltd/asm-polycall/include/asm_polycall.h'),
  binding.publicHeader
);

console.log('asm-polycall npm package test: PASS');
