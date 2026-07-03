'use strict';

const path = require('node:path');

const fromPackageRoot = (...segments) => path.join(__dirname, ...segments);

module.exports = Object.freeze({
  root: __dirname,
  assembly: fromPackageRoot('src', 'asm_polycall.S'),
  publicHeader: fromPackageRoot('include', 'asm_polycall.h'),
  ffiHeader: fromPackageRoot('generated', 'polycall', 'polycall_ffi.h'),
  config: fromPackageRoot('asm-polycallrc'),
  manifest: fromPackageRoot('polycall-binding.json')
});
